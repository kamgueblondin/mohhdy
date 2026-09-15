#!/usr/bin/env python3
"""Runtime HTTP agent MOHHDY (ASSIST-050 + 010..022/030/031/040/041).

Sert l'origine d'embed, des sessions visiteur isolees, une KB locale, un
masque de droits, l'escalade, le handoff, un simulateur de gestes DOM sur
une origine allowlistee, des outils MCP declares et une appli hote mock
(facture). Ce n'est pas un LLM de production, pas d'appel OpenAI, pas
Chromium, et ce n'est pas le noyau Multiboot i386.
"""

from __future__ import annotations

import hmac
import json
import os
import posixpath
import re
import sys
import threading
import uuid
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any, Optional
from urllib.parse import parse_qs, unquote, urlparse

import tools as agent_tools

SERVICE_NAME = "mohhdy-agent"
DEFAULT_HOST = "0.0.0.0"
DEFAULT_PORT = 8080
LLM_KIND = "stub_echo"
LLM_KB = "stub_kb"
LLM_REFUSAL = "stub_refusal"

ROOT = Path(__file__).resolve().parent
STATIC_DIR = ROOT / "static"

MAX_BODY_BYTES = 16384
MAX_CONTENT_CHARS = 4000
MAX_MESSAGES = 100
MAX_SESSIONS = 500
SITE_ID_RE = re.compile(r"^[A-Za-z0-9._-]{1,64}$")
SESSION_ID_RE = re.compile(
    r"^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"
)
TOOL_RE = re.compile(r"^[A-Za-z][A-Za-z0-9._-]{0,63}$")
TOKEN_RE = re.compile(r"[a-z0-9]+", re.IGNORECASE)

STATUS_OPEN = "open"
STATUS_WAITING = "waiting_human"
STATUS_HUMAN = "human_active"
STATUS_CLOSED = "closed"
VALID_STATUSES = (STATUS_OPEN, STATUS_WAITING, STATUS_HUMAN, STATUS_CLOSED)

CAP_CHAT = "chat.reply"
CAP_EXPLAIN = "site.explain"
CAP_ESCALATE = "session.escalate"
CAP_OBSERVE = "admin.observe"
CAP_TAKEOVER = "admin.takeover"
CAP_DOM_CLICK = agent_tools.CAP_DOM_CLICK
CAP_DOM_TYPE = agent_tools.CAP_DOM_TYPE
CAP_POINTER = agent_tools.CAP_POINTER
CAP_INVOICE = agent_tools.CAP_INVOICE

VISITOR_CAPABILITIES = (CAP_CHAT, CAP_EXPLAIN, CAP_ESCALATE)
ADMIN_CAPABILITIES = (CAP_OBSERVE, CAP_TAKEOVER)
GESTURE_CAPABILITIES = agent_tools.GESTURE_TOOLS
KNOWN_CAPABILITIES = (
    VISITOR_CAPABILITIES + ADMIN_CAPABILITIES + GESTURE_CAPABILITIES
)
HARNESS_KIND = agent_tools.HARNESS_KIND
INTERNAL_PREFIXES = ("acl.", "internal.")

DEFAULT_CAPABILITIES = [
    CAP_CHAT,
    CAP_EXPLAIN,
    CAP_ESCALATE,
    CAP_OBSERVE,
    CAP_TAKEOVER,
]

STOPWORDS = {
    "les",
    "des",
    "une",
    "un",
    "le",
    "la",
    "et",
    "ou",
    "de",
    "du",
    "en",
    "au",
    "aux",
    "ce",
    "cet",
    "cette",
    "ces",
    "qui",
    "que",
    "quoi",
    "pas",
    "pour",
    "par",
    "sur",
    "dans",
    "est",
    "sont",
    "avec",
    "sans",
    "plus",
    "comment",
    "quoi",
}

TEXT_HEADERS = {
    "X-Content-Type-Options": "nosniff",
    "Referrer-Policy": "no-referrer",
    "Cache-Control": "no-store",
}

PUBLIC_JS_HEADERS = {
    "X-Content-Type-Options": "nosniff",
    "Referrer-Policy": "no-referrer",
    "Cache-Control": "public, max-age=60",
    "Access-Control-Allow-Origin": "*",
}

PUBLIC_CSS_HEADERS = {
    "X-Content-Type-Options": "nosniff",
    "Referrer-Policy": "no-referrer",
    "Cache-Control": "public, max-age=60",
    "Access-Control-Allow-Origin": "*",
}

VISITOR_API_HEADERS = {
    "X-Content-Type-Options": "nosniff",
    "Referrer-Policy": "no-referrer",
    "Cache-Control": "no-store",
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type",
}


class CapabilityDenied(PermissionError):
    def __init__(
        self, capability: str, tool: Optional[str] = None, request_id: str = ""
    ) -> None:
        super().__init__("capability_denied")
        self.capability = capability
        self.tool = tool
        self.request_id = request_id


class SessionConflict(PermissionError):
    def __init__(self, code: str) -> None:
        super().__init__(code)
        self.code = code


def utc_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def env_host() -> str:
    return os.environ.get("MOHHDY_AGENT_HOST", DEFAULT_HOST)


def env_port() -> int:
    raw = os.environ.get("MOHHDY_AGENT_PORT", str(DEFAULT_PORT))
    try:
        port = int(raw)
    except ValueError as exc:
        raise SystemExit("MOHHDY_AGENT_PORT doit etre un entier") from exc
    if not (1 <= port <= 65535):
        raise SystemExit("MOHHDY_AGENT_PORT hors plage 1-65535")
    return port


def env_admin_token() -> str:
    return os.environ.get("ADMIN_TOKEN") or ""


def env_data_dir() -> Optional[Path]:
    raw = (os.environ.get("MOHHDY_AGENT_DATA") or "").strip()
    if not raw:
        return None
    path = Path(raw)
    try:
        path.mkdir(parents=True, exist_ok=True)
    except OSError:
        return None
    return path


def env_data_path() -> Optional[Path]:
    directory = env_data_dir()
    if directory is None:
        return None
    return directory / "sessions.json"


def admin_auth_mode() -> str:
    return "token" if env_admin_token() else "open_stub"


def normalize_site_id(raw: Any) -> str:
    if raw is None:
        return "unspecified"
    text = str(raw).strip()
    if not text:
        return "unspecified"
    if not SITE_ID_RE.match(text):
        raise ValueError("site_id invalide")
    return text


def is_internal_capability(name: str) -> bool:
    lowered = name.lower()
    return lowered.startswith(INTERNAL_PREFIXES)


def is_placeholder_tool(name: str) -> bool:
    """Outil accorde mais sans runner (MCP non declare/implemente hors demo)."""
    if name in GESTURE_CAPABILITIES or name == CAP_INVOICE:
        return False
    return name.startswith("mcp.") or name.startswith("dom.")


def validate_capability_name(name: Any) -> str:
    if not isinstance(name, str):
        raise ValueError("capacite invalide")
    text = name.strip()
    if not text or not TOOL_RE.match(text):
        raise ValueError("capacite invalide")
    if is_internal_capability(text):
        raise ValueError("capacite interne interdite")
    if text in KNOWN_CAPABILITIES or text.startswith("mcp."):
        return text
    raise ValueError("capacite inconnue")


def public_capabilities(caps: list[str]) -> list[str]:
    visible = []
    for name in caps:
        if name in ADMIN_CAPABILITIES or is_internal_capability(name):
            continue
        visible.append(name)
    return visible


def capability_catalog(policy: Optional["PolicyStore"] = None) -> list[dict[str, str]]:
    rows = []
    for name in KNOWN_CAPABILITIES:
        if name in GESTURE_CAPABILITIES:
            kind = "gesture"
        elif name in ADMIN_CAPABILITIES:
            kind = "admin"
        else:
            kind = "session"
        rows.append({"name": name, "kind": kind})
    declared = (
        policy.declared_tools_public()
        if policy is not None
        else dict(agent_tools.DEFAULT_DECLARED_TOOLS)
    )
    for name, spec in declared.items():
        rows.append(
            {
                "name": name,
                "kind": spec.get("kind") or "mcp",
                "description": spec.get("description") or "",
            }
        )
    rows.append({"name": "mcp.*", "kind": "allowlist"})
    return rows


def tokenize(text: str) -> set[str]:
    tokens = set()
    for raw in TOKEN_RE.findall(text.lower()):
        if len(raw) < 3 or raw in STOPWORDS:
            continue
        tokens.add(raw)
    return tokens


def looks_like_explain(text: str) -> bool:
    lowered = text.lower()
    hints = (
        "comment",
        "marche",
        "expliquer",
        "explique",
        "plateforme",
        "parcours",
        "offre",
        "limites",
        "comment ca",
        "c'est quoi",
        "cest quoi",
    )
    return any(hint in lowered for hint in hints)


def stub_reply(text: str) -> str:
    excerpt = text.strip()
    if len(excerpt) > 240:
        excerpt = excerpt[:240] + "..."
    return (
        "Reponse stub (pas un LLM de production, aucun appel reseau). "
        "Vous avez dit : %s" % excerpt
    )


def grounded_reply(entries: list[dict[str, str]], question: str) -> tuple[str, str]:
    """Reponse ancree dans la KB. Ne invente pas hors extraits."""
    if not entries:
        return (
            "Aucune base de connaissance autorisee n'est configuree pour ce site. "
            "Je ne peux pas inventer une explication.",
            LLM_REFUSAL,
        )
    q_tokens = tokenize(question)
    scored: list[tuple[int, dict[str, str]]] = []
    for entry in entries:
        blob = "%s %s" % (entry.get("title") or "", entry.get("text") or "")
        overlap = len(q_tokens & tokenize(blob)) if q_tokens else 0
        scored.append((overlap, entry))
    scored.sort(key=lambda item: item[0], reverse=True)
    best_score, best = scored[0]
    titles = [item["title"] for _, item in scored if item.get("title")]
    if best_score > 0:
        body = (best.get("text") or "").strip()
        title = (best.get("title") or "").strip()
        prefix = "D'apres la base autorisee"
        if title:
            prefix += " (%s)" % title
        return (
            "%s : %s (reponse locale stub, pas un LLM de production)."
            % (prefix, body),
            LLM_KB,
        )
    listing = "; ".join(titles) if titles else "entrees sans titre"
    first = (entries[0].get("text") or "").strip()
    return (
        "La base autorisee ne contient pas de passage precis pour cette question. "
        "Sujets disponibles : %s. Extrait : %s "
        "(reponse locale stub, pas un LLM de production)."
        % (listing, first),
        LLM_KB,
    )


def parse_kb_payload(raw: Any) -> list[dict[str, str]]:
    entries: list[dict[str, str]] = []
    if isinstance(raw, list):
        for item in raw:
            parsed = _kb_entry(item)
            if parsed is not None:
                entries.append(parsed)
        return entries
    if isinstance(raw, dict):
        if "kb" in raw:
            return parse_kb_payload(raw.get("kb"))
        parsed = _kb_entry(raw)
        if parsed is not None:
            entries.append(parsed)
        return entries
    if isinstance(raw, str):
        text = raw.strip()
        if text:
            entries.append({"id": "kb", "title": "Base autorisee", "text": text})
        return entries
    return entries


def _kb_entry(item: Any) -> Optional[dict[str, str]]:
    if isinstance(item, str):
        text = item.strip()
        if not text:
            return None
        return {"id": "kb", "title": "Base autorisee", "text": text}
    if not isinstance(item, dict):
        return None
    text = str(item.get("text") or item.get("body") or item.get("content") or "").strip()
    if not text:
        return None
    title = str(item.get("title") or item.get("id") or "Base autorisee").strip()
    entry_id = str(item.get("id") or title).strip() or "kb"
    return {"id": entry_id[:64], "title": title[:120], "text": text[:4000]}


def load_kb_file(path: Path) -> list[dict[str, str]]:
    if not path.is_file():
        return []
    try:
        raw_text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        return []
    stripped = raw_text.strip()
    if not stripped:
        return []
    if path.suffix.lower() == ".json" or stripped[:1] in "{[":
        try:
            payload = json.loads(stripped)
        except json.JSONDecodeError:
            payload = stripped
        return parse_kb_payload(payload)
    chunks: list[dict[str, str]] = []
    current_title = path.stem
    current_lines: list[str] = []
    for line in raw_text.splitlines():
        if line.startswith("## "):
            body = "\n".join(current_lines).strip()
            if body:
                chunks.append(
                    {"id": current_title[:64], "title": current_title[:120], "text": body[:4000]}
                )
            current_title = line[3:].strip() or path.stem
            current_lines = []
            continue
        current_lines.append(line)
    body = "\n".join(current_lines).strip()
    if body:
        chunks.append(
            {"id": current_title[:64], "title": current_title[:120], "text": body[:4000]}
        )
    return chunks


class PolicyStore:
    """Allowlist de site, KB locale, masques de droits (memoire)."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self.default_capabilities = list(DEFAULT_CAPABILITIES)
        self._site_capabilities: dict[str, list[str]] = {}
        self._site_kb: dict[str, list[dict[str, str]]] = {}
        self._default_kb: list[dict[str, str]] = []
        self.allowed_origins = list(agent_tools.DEFAULT_ORIGINS)
        self._site_origins: dict[str, list[str]] = {}
        self._declared_tools: dict[str, dict[str, str]] = dict(
            agent_tools.DEFAULT_DECLARED_TOOLS
        )
        self.config_loaded = False
        self.kb_loaded = False
        self._config_path = ""
        self._kb_path = ""

    def load_from_env(self) -> None:
        config_raw = (os.environ.get("MOHHDY_AGENT_CONFIG") or "").strip()
        kb_raw = (os.environ.get("MOHHDY_AGENT_KB") or "").strip()
        self._config_path = config_raw
        self._kb_path = kb_raw
        if config_raw:
            self.apply_config_file(Path(config_raw))
        if kb_raw:
            self.apply_kb_file(Path(kb_raw))

    def reset_runtime(self) -> None:
        with self._lock:
            self.default_capabilities = list(DEFAULT_CAPABILITIES)
            self._site_capabilities = {}
            self._site_kb = {}
            self._default_kb = []
            self.allowed_origins = list(agent_tools.DEFAULT_ORIGINS)
            self._site_origins = {}
            self._declared_tools = dict(agent_tools.DEFAULT_DECLARED_TOOLS)
            self.config_loaded = False
            self.kb_loaded = False
        self.load_from_env()

    def apply_config_file(self, path: Path) -> None:
        if not path.is_file():
            return
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError, UnicodeDecodeError):
            return
        if not isinstance(payload, dict):
            return
        with self._lock:
            defaults = payload.get("default_capabilities")
            if isinstance(defaults, list):
                parsed = []
                for item in defaults:
                    try:
                        parsed.append(validate_capability_name(item))
                    except ValueError:
                        continue
                if parsed:
                    self.default_capabilities = parsed
            origins = agent_tools.parse_origin_list(payload.get("allowed_origins"))
            if origins:
                self.allowed_origins = origins
            if "tools" in payload:
                self._declared_tools = agent_tools.parse_declared_tools(
                    payload.get("tools")
                )
            sites = payload.get("sites")
            if isinstance(sites, dict):
                for site_id, spec in sites.items():
                    try:
                        norm = normalize_site_id(site_id)
                    except ValueError:
                        continue
                    if not isinstance(spec, dict):
                        continue
                    caps = spec.get("capabilities")
                    if isinstance(caps, list):
                        parsed_caps = []
                        for item in caps:
                            try:
                                parsed_caps.append(validate_capability_name(item))
                            except ValueError:
                                continue
                        if parsed_caps:
                            self._site_capabilities[norm] = parsed_caps
                    kb_entries = parse_kb_payload(spec.get("kb"))
                    kb_file = spec.get("kb_file")
                    if isinstance(kb_file, str) and kb_file.strip():
                        kb_path = Path(kb_file)
                        if not kb_path.is_absolute():
                            kb_path = path.parent / kb_path
                        kb_entries = kb_entries + load_kb_file(kb_path)
                    if kb_entries:
                        self._site_kb[norm] = kb_entries
                        self.kb_loaded = True
                    site_origins = agent_tools.parse_origin_list(
                        spec.get("allowed_origins")
                    )
                    if site_origins:
                        self._site_origins[norm] = site_origins
            top_kb = parse_kb_payload(payload.get("kb"))
            if top_kb:
                self._default_kb = top_kb
                self.kb_loaded = True
            self.config_loaded = True

    def apply_kb_file(self, path: Path) -> None:
        entries = load_kb_file(path)
        if not entries:
            return
        with self._lock:
            self._default_kb = list(self._default_kb) + entries
            self.kb_loaded = True

    def capabilities_for(self, site_id: str) -> list[str]:
        with self._lock:
            caps = self._site_capabilities.get(site_id)
            if caps:
                return list(caps)
            return list(self.default_capabilities)

    def kb_for(self, site_id: str) -> list[dict[str, str]]:
        with self._lock:
            entries = list(self._site_kb.get(site_id) or [])
            if not entries:
                entries = list(self._default_kb)
            return [dict(item) for item in entries]

    def kb_present(self, site_id: str) -> bool:
        return bool(self.kb_for(site_id))

    def set_site_capabilities(self, site_id: str, caps: list[str]) -> list[str]:
        unique = list(dict.fromkeys(caps))
        with self._lock:
            self._site_capabilities[site_id] = unique
            return list(unique)

    def grant_site(self, site_id: str, names: list[str]) -> list[str]:
        with self._lock:
            current = list(
                self._site_capabilities.get(site_id) or self.default_capabilities
            )
            for name in names:
                if name not in current:
                    current.append(name)
            self._site_capabilities[site_id] = current
            return list(current)

    def revoke_site(self, site_id: str, names: list[str]) -> list[str]:
        drop = set(names)
        with self._lock:
            current = list(
                self._site_capabilities.get(site_id) or self.default_capabilities
            )
            current = [item for item in current if item not in drop]
            self._site_capabilities[site_id] = current
            return list(current)

    def origins_for(self, site_id: str) -> list[str]:
        with self._lock:
            site_origins = self._site_origins.get(site_id)
            if site_origins:
                return list(site_origins)
            return list(self.allowed_origins)

    def origin_allowed(self, site_id: str, origin: str, self_origin: str) -> bool:
        return agent_tools.origin_allowed(
            origin, self.origins_for(site_id), self_origin
        )

    def tool_declared(self, name: str) -> bool:
        with self._lock:
            return name in self._declared_tools

    def declared_tools_public(self) -> dict[str, dict[str, str]]:
        with self._lock:
            return {
                name: dict(spec) for name, spec in self._declared_tools.items()
            }

    def public_status(self) -> dict[str, Any]:
        with self._lock:
            sites = sorted(set(self._site_capabilities) | set(self._site_kb))
            return {
                "config_loaded": self.config_loaded,
                "kb_loaded": self.kb_loaded or bool(self._default_kb) or bool(self._site_kb),
                "kb_sites": sorted(
                    site for site in set(self._site_kb) if self._site_kb.get(site)
                ),
                "default_kb": bool(self._default_kb),
                "sites": sites,
                "declared_tools": sorted(self._declared_tools),
                "allowed_origins": list(self.allowed_origins),
                "harness": HARNESS_KIND,
            }


def compose_agent_reply(
    policy: PolicyStore, site_id: str, caps: list[str], content: str
) -> tuple[str, str]:
    has_chat = CAP_CHAT in caps
    has_explain = CAP_EXPLAIN in caps
    kb_entries = policy.kb_for(site_id)
    explain = looks_like_explain(content)

    if not has_chat:
        raise CapabilityDenied(CAP_CHAT)

    if kb_entries:
        if not has_explain:
            return (
                "Le droit site.explain n'est pas accorde. Je ne peux pas expliquer "
                "la plateforme ni inventer un contenu hors allowlist.",
                LLM_REFUSAL,
            )
        return grounded_reply(kb_entries, content)

    if explain:
        return grounded_reply([], content)

    return stub_reply(content), LLM_KIND


def safe_static_path(name: str) -> Optional[Path]:
    """Resout un fichier sous static/, sans traversal."""
    if not name or name.startswith(".") or "/" in name or "\\" in name:
        return None
    candidate = (STATIC_DIR / name).resolve()
    try:
        candidate.relative_to(STATIC_DIR.resolve())
    except ValueError:
        return None
    if not candidate.is_file():
        return None
    return candidate


def _message(
    role: str,
    content: str,
    request_id: str,
    kind: str,
    extra: Optional[dict[str, Any]] = None,
) -> dict[str, Any]:
    payload = {
        "id": str(uuid.uuid4()),
        "role": role,
        "speaker": role,
        "content": content,
        "created_at": utc_now(),
        "request_id": request_id,
        "kind": kind,
    }
    if extra:
        payload.update(extra)
    return payload


class SessionStore:
    """Sessions en memoire, optionnellement relues/ecrites en JSON."""

    def __init__(
        self,
        persist_path: Optional[Path] = None,
        policy: Optional[PolicyStore] = None,
        harness: Optional[agent_tools.DemoHarness] = None,
        invoices: Optional[agent_tools.InvoiceStore] = None,
        journal: Optional[agent_tools.AuditJournal] = None,
    ) -> None:
        self._lock = threading.Lock()
        self._sessions: dict[str, dict[str, Any]] = {}
        self._persist_path = persist_path
        self.policy = policy if policy is not None else PolicyStore()
        self.harness = harness if harness is not None else agent_tools.DemoHarness()
        self.invoices = invoices if invoices is not None else agent_tools.InvoiceStore()
        self.journal = journal if journal is not None else agent_tools.AuditJournal()
        self.runner = agent_tools.ToolRunner(self.harness, self.invoices)
        if persist_path is not None:
            self._load_unlocked()

    def clear(self) -> None:
        with self._lock:
            self._sessions = {}
            self._persist_unlocked()

    def create(self, site_id: str) -> dict[str, Any]:
        with self._lock:
            if len(self._sessions) >= MAX_SESSIONS:
                raise OverflowError("trop de sessions")
            session_id = str(uuid.uuid4())
            now = utc_now()
            record = {
                "session_id": session_id,
                "site_id": site_id,
                "status": STATUS_OPEN,
                "created_at": now,
                "updated_at": now,
                "messages": [],
                "capabilities": self.policy.capabilities_for(site_id),
                "handoff": False,
                "escalate_reason": None,
                "escalated_at": None,
                "taken_over_at": None,
            }
            self._sessions[session_id] = record
            self._persist_unlocked()
            return self._public_session(record, include_messages=True, admin=False)

    def get(
        self,
        session_id: str,
        include_messages: bool = True,
        admin: bool = False,
    ) -> Optional[dict[str, Any]]:
        with self._lock:
            record = self._sessions.get(session_id)
            if record is None:
                return None
            return self._public_session(
                record, include_messages=include_messages, admin=admin
            )

    def list_sessions(
        self,
        site_id: Optional[str] = None,
        status: Optional[str] = None,
        admin: bool = True,
    ) -> list[dict[str, Any]]:
        with self._lock:
            rows = []
            for record in self._sessions.values():
                if site_id is not None and record["site_id"] != site_id:
                    continue
                if status is not None and record.get("status") != status:
                    continue
                rows.append(
                    self._public_session(record, include_messages=False, admin=admin)
                )
            rows.sort(key=lambda item: item["updated_at"], reverse=True)
            return rows

    def add_visitor_message(self, session_id: str, content: str) -> dict[str, Any]:
        with self._lock:
            record = self._require(session_id)
            if record["status"] == STATUS_CLOSED:
                raise SessionConflict("session_closed")
            auto_reply = record["status"] == STATUS_OPEN
            extra = 2 if auto_reply else 1
            if len(record["messages"]) + extra > MAX_MESSAGES:
                raise OverflowError("trop de messages")
            request_id = str(uuid.uuid4())
            visitor_msg = _message("visitor", content, request_id, "user")
            record["messages"].append(visitor_msg)
            agent_msg = None
            llm = LLM_KIND
            if auto_reply:
                caps = list(record.get("capabilities") or [])
                try:
                    text, llm = compose_agent_reply(
                        self.policy, record["site_id"], caps, content
                    )
                except CapabilityDenied:
                    text = (
                        "Le droit chat.reply n'est pas accorde. Je ne peux pas repondre "
                        "ni inventer un droit pour aider quand meme."
                    )
                    llm = LLM_REFUSAL
                agent_msg = _message("agent", text, request_id, llm)
                record["messages"].append(agent_msg)
            record["updated_at"] = utc_now()
            self._persist_unlocked()
            payload = {
                "session_id": session_id,
                "request_id": request_id,
                "llm": llm if auto_reply else None,
                "auto_reply": auto_reply,
                "status": record["status"],
                "visitor_message": dict(visitor_msg),
                "agent_message": dict(agent_msg) if agent_msg else None,
            }
            return payload

    def escalate(
        self, session_id: str, reason: str, actor: str, force: bool = False
    ) -> dict[str, Any]:
        with self._lock:
            record = self._require(session_id)
            if record["status"] == STATUS_CLOSED:
                raise SessionConflict("session_closed")
            caps = list(record.get("capabilities") or [])
            if not force and actor == "visitor" and CAP_ESCALATE not in caps:
                raise CapabilityDenied(CAP_ESCALATE)
            if record["status"] in (STATUS_WAITING, STATUS_HUMAN):
                return self._public_session(record, include_messages=True, admin=True)
            if len(record["messages"]) + 1 > MAX_MESSAGES:
                raise OverflowError("trop de messages")
            request_id = str(uuid.uuid4())
            now = utc_now()
            if actor == "policy":
                text = (
                    "Demande hors allowlist : aucun droit invente, acte non execute. "
                    "Session placee en attente d'un humain."
                )
            else:
                text = "Le visiteur a demande a parler a un humain."
                if reason:
                    text += " Motif : %s" % reason
            record["messages"].append(_message("system", text, request_id, "escalate"))
            record["status"] = STATUS_WAITING
            record["escalate_reason"] = actor
            record["escalated_at"] = now
            record["updated_at"] = now
            self._persist_unlocked()
            return self._public_session(record, include_messages=True, admin=True)

    def invoke_tool(
        self,
        session_id: str,
        tool: str,
        origin: str = "",
        args: Optional[dict[str, Any]] = None,
        self_origin: str = "",
    ) -> dict[str, Any]:
        tool = validate_capability_name(tool)
        payload_args = args if isinstance(args, dict) else {}
        with self._lock:
            record = self._require(session_id)
            if record["status"] == STATUS_CLOSED:
                raise SessionConflict("session_closed")
            caps = list(record.get("capabilities") or [])
            request_id = str(uuid.uuid4())
            site_id = record["site_id"]

            def deny_message(text: str, kind: str) -> None:
                if len(record["messages"]) + 1 > MAX_MESSAGES:
                    raise OverflowError("trop de messages")
                record["messages"].append(
                    _message(
                        "system",
                        text,
                        request_id,
                        kind,
                        extra={"tool": tool, "origin": origin},
                    )
                )
                if record["status"] == STATUS_OPEN:
                    record["status"] = STATUS_WAITING
                    record["escalate_reason"] = "policy"
                    record["escalated_at"] = utc_now()
                record["updated_at"] = utc_now()
                self._persist_unlocked()

            if tool not in caps:
                self.journal.record(
                    request_id, session_id, site_id, tool, origin, "capability_denied"
                )
                deny_message(
                    "Outil refuse (%s) : absent de l'allowlist. "
                    "L'agent n'invente pas ce droit. Aucun acte execute. "
                    "Escalade vers un humain. request_id=%s"
                    % (tool, request_id),
                    "tool_denied",
                )
                raise CapabilityDenied(tool, tool=tool, request_id=request_id)

            if agent_tools.is_mcp_tool(tool) and not self.policy.tool_declared(tool):
                self.journal.record(
                    request_id, session_id, site_id, tool, origin, "undeclared"
                )
                deny_message(
                    "Outil %s non declare dans la config operateur. "
                    "Appel refuse. Escalade vers un humain. request_id=%s"
                    % (tool, request_id),
                    "tool_undeclared",
                )
                raise agent_tools.ToolUndeclared(tool, request_id)

            needs_origin = agent_tools.is_gesture_tool(tool) or agent_tools.is_mcp_tool(
                tool
            )
            if needs_origin:
                if not origin:
                    origin = self_origin
                if not origin or not self.policy.origin_allowed(
                    site_id, origin, self_origin
                ):
                    self.journal.record(
                        request_id,
                        session_id,
                        site_id,
                        tool,
                        origin,
                        "origin_denied",
                    )
                    deny_message(
                        "Origine refusee (%s) pour %s. Hors allowlist. "
                        "Aucun acte execute. request_id=%s"
                        % (origin or "(vide)", tool, request_id),
                        "origin_denied",
                    )
                    raise agent_tools.OriginDenied(origin, request_id)

            if is_placeholder_tool(tool):
                self.journal.record(
                    request_id, session_id, site_id, tool, origin, "not_implemented"
                )
                if len(record["messages"]) + 1 > MAX_MESSAGES:
                    raise OverflowError("trop de messages")
                text = (
                    "Outil %s accorde mais non implemente. "
                    "Aucun acte n'a ete execute. request_id=%s"
                    % (tool, request_id)
                )
                record["messages"].append(
                    _message(
                        "system",
                        text,
                        request_id,
                        "tool_placeholder",
                        extra={"tool": tool},
                    )
                )
                record["updated_at"] = utc_now()
                self._persist_unlocked()
                raise SessionConflict("tool_not_implemented")

            try:
                executed = self.runner.execute(
                    tool, payload_args, request_id, session_id, site_id
                )
            except agent_tools.ToolArgsError as exc:
                if not exc.request_id:
                    exc.request_id = request_id
                self.journal.record(
                    request_id,
                    session_id,
                    site_id,
                    tool,
                    origin,
                    "bad_args",
                    extra={"message": str(exc)},
                )
                if len(record["messages"]) + 1 > MAX_MESSAGES:
                    raise OverflowError("trop de messages")
                record["messages"].append(
                    _message(
                        "system",
                        "Outil %s refuse (arguments invalides). request_id=%s"
                        % (tool, request_id),
                        request_id,
                        "tool_bad_args",
                        extra={"tool": tool},
                    )
                )
                record["updated_at"] = utc_now()
                self._persist_unlocked()
                raise
            except OverflowError:
                raise

            self.journal.record(
                request_id, session_id, site_id, tool, origin, "ok"
            )
            if len(record["messages"]) + 1 > MAX_MESSAGES:
                raise OverflowError("trop de messages")
            if agent_tools.is_gesture_tool(tool):
                summary = (
                    "Geste %s execute (%s) sur %s. request_id=%s"
                    % (tool, HARNESS_KIND, origin, request_id)
                )
            elif tool == CAP_INVOICE:
                invoice = executed.get("invoice") or {}
                summary = (
                    "Facture %s creee dans l'appli hote mock. "
                    "session_id=%s request_id=%s"
                    % (invoice.get("invoice_id"), session_id, request_id)
                )
            else:
                summary = "Outil %s execute. request_id=%s" % (tool, request_id)
            record["messages"].append(
                _message(
                    "system",
                    summary,
                    request_id,
                    "tool_ok",
                    extra={"tool": tool, "origin": origin},
                )
            )
            record["updated_at"] = utc_now()
            self._persist_unlocked()
            return {
                "ok": True,
                "request_id": request_id,
                "session_id": session_id,
                "site_id": site_id,
                "tool": tool,
                "origin": origin,
                "harness": HARNESS_KIND,
                "result": executed,
            }

    def takeover(self, session_id: str) -> dict[str, Any]:
        with self._lock:
            record = self._require(session_id)
            if record["status"] == STATUS_CLOSED:
                raise SessionConflict("session_closed")
            caps = list(record.get("capabilities") or [])
            if CAP_TAKEOVER not in caps:
                raise CapabilityDenied(CAP_TAKEOVER)
            if record["status"] == STATUS_HUMAN and record.get("handoff"):
                return self._public_session(record, include_messages=True, admin=True)
            if len(record["messages"]) + 1 > MAX_MESSAGES:
                raise OverflowError("trop de messages")
            request_id = str(uuid.uuid4())
            now = utc_now()
            record["messages"].append(
                _message(
                    "system",
                    "Un humain a pris la main sur cette session. L'agent ne repond plus automatiquement.",
                    request_id,
                    "handoff",
                )
            )
            record["status"] = STATUS_HUMAN
            record["handoff"] = True
            record["taken_over_at"] = now
            record["updated_at"] = now
            self._persist_unlocked()
            return self._public_session(record, include_messages=True, admin=True)

    def add_human_message(self, session_id: str, content: str) -> dict[str, Any]:
        with self._lock:
            record = self._require(session_id)
            if record["status"] == STATUS_CLOSED:
                raise SessionConflict("session_closed")
            caps = list(record.get("capabilities") or [])
            if CAP_TAKEOVER not in caps:
                raise CapabilityDenied(CAP_TAKEOVER)
            if not record.get("handoff") or record["status"] != STATUS_HUMAN:
                if CAP_TAKEOVER not in caps:
                    raise CapabilityDenied(CAP_TAKEOVER)
                if len(record["messages"]) + 2 > MAX_MESSAGES:
                    raise OverflowError("trop de messages")
                request_id_take = str(uuid.uuid4())
                now = utc_now()
                record["messages"].append(
                    _message(
                        "system",
                        "Un humain a pris la main sur cette session. L'agent ne repond plus automatiquement.",
                        request_id_take,
                        "handoff",
                    )
                )
                record["status"] = STATUS_HUMAN
                record["handoff"] = True
                record["taken_over_at"] = now
            elif len(record["messages"]) + 1 > MAX_MESSAGES:
                raise OverflowError("trop de messages")
            request_id = str(uuid.uuid4())
            human_msg = _message("human", content, request_id, "human")
            record["messages"].append(human_msg)
            record["updated_at"] = utc_now()
            self._persist_unlocked()
            return {
                "session_id": session_id,
                "request_id": request_id,
                "status": record["status"],
                "handoff": True,
                "human_message": dict(human_msg),
            }

    def set_capabilities(
        self,
        session_id: str,
        grant: Optional[list[str]] = None,
        revoke: Optional[list[str]] = None,
        replace: Optional[list[str]] = None,
    ) -> dict[str, Any]:
        with self._lock:
            record = self._require(session_id)
            current = list(record.get("capabilities") or [])
            if replace is not None:
                current = list(dict.fromkeys(replace))
            if grant:
                for name in grant:
                    if name not in current:
                        current.append(name)
            if revoke:
                drop = set(revoke)
                current = [item for item in current if item not in drop]
            record["capabilities"] = current
            record["updated_at"] = utc_now()
            self._persist_unlocked()
            return self._public_session(record, include_messages=False, admin=True)

    def _require(self, session_id: str) -> dict[str, Any]:
        record = self._sessions.get(session_id)
        if record is None:
            raise KeyError("session_id inconnu")
        self._normalize_record(record)
        return record

    @staticmethod
    def _normalize_record(record: dict[str, Any]) -> None:
        record.setdefault("status", STATUS_OPEN)
        record.setdefault("messages", [])
        record.setdefault("capabilities", list(DEFAULT_CAPABILITIES))
        record.setdefault("handoff", record.get("status") == STATUS_HUMAN)
        record.setdefault("escalate_reason", None)
        record.setdefault("escalated_at", None)
        record.setdefault("taken_over_at", None)

    def _public_session(
        self, record: dict[str, Any], include_messages: bool, admin: bool
    ) -> dict[str, Any]:
        self._normalize_record(record)
        caps = list(record.get("capabilities") or [])
        payload = {
            "session_id": record["session_id"],
            "site_id": record["site_id"],
            "status": record["status"],
            "created_at": record["created_at"],
            "updated_at": record["updated_at"],
            "message_count": len(record["messages"]),
            "handoff": bool(record.get("handoff")),
            "escalate_reason": record.get("escalate_reason"),
            "capabilities": caps if admin else public_capabilities(caps),
            "kb_available": self.policy.kb_present(record["site_id"]),
        }
        if include_messages:
            payload["messages"] = [dict(item) for item in record["messages"]]
        return payload

    def _load_unlocked(self) -> None:
        path = self._persist_path
        if path is None or not path.is_file():
            return
        try:
            raw = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError, UnicodeDecodeError):
            return
        sessions = raw.get("sessions") if isinstance(raw, dict) else None
        if not isinstance(sessions, dict):
            return
        restored: dict[str, dict[str, Any]] = {}
        for session_id, record in sessions.items():
            if not isinstance(session_id, str) or not SESSION_ID_RE.match(session_id):
                continue
            if not isinstance(record, dict):
                continue
            restored[session_id] = record
        self._sessions = restored

    def _persist_unlocked(self) -> None:
        path = self._persist_path
        if path is None:
            return
        payload = json.dumps({"sessions": self._sessions}, separators=(",", ":"))
        tmp = path.with_suffix(".json.tmp")
        try:
            tmp.write_text(payload, encoding="utf-8")
            tmp.replace(path)
        except OSError:
            try:
                tmp.unlink()
            except OSError:
                pass


class AgentHTTPServer(ThreadingHTTPServer):
    def __init__(self, server_address, RequestHandlerClass):
        super().__init__(server_address, RequestHandlerClass)
        policy = PolicyStore()
        policy.load_from_env()
        self.policy = policy
        data_dir = env_data_dir()
        invoices_path = data_dir / "invoices.json" if data_dir is not None else None
        journal_path = data_dir / "journal.json" if data_dir is not None else None
        self.harness = agent_tools.DemoHarness()
        self.invoices = agent_tools.InvoiceStore(invoices_path)
        self.journal = agent_tools.AuditJournal(journal_path)
        self.store = SessionStore(
            env_data_path(),
            policy=policy,
            harness=self.harness,
            invoices=self.invoices,
            journal=self.journal,
        )

    def reset_runtime(self) -> None:
        self.store.clear()
        self.policy.reset_runtime()
        self.harness.reset()
        self.invoices.clear()
        self.journal.clear()


class AgentHandler(BaseHTTPRequestHandler):
    server_version = "MOHHDY-Agent/0.4"

    def log_message(self, fmt: str, *args) -> None:
        sys.stderr.write(
            "mohhdy-agent: %s - %s\n" % (self.address_string(), fmt % args)
        )

    def do_GET(self) -> None:
        self._dispatch(method="GET", send_body=True)

    def do_HEAD(self) -> None:
        self._dispatch(method="HEAD", send_body=False)

    def do_POST(self) -> None:
        self._dispatch(method="POST", send_body=True)

    def do_OPTIONS(self) -> None:
        parsed = urlparse(self.path)
        path = self._normalize(parsed.path)
        if path in ("/embed.js", "/embed.css") or path.startswith("/api/sessions") or path.startswith("/api/demo-app"):
            self.send_response(204)
            if path.startswith("/api/sessions") or path.startswith("/api/demo-app"):
                self._write_headers(VISITOR_API_HEADERS)
            elif path == "/embed.css":
                self._write_headers(PUBLIC_CSS_HEADERS)
            else:
                self._write_headers(PUBLIC_JS_HEADERS)
            self.end_headers()
            return
        self._send_json_status(404, {"status": "not_found", "service": SERVICE_NAME})

    def _dispatch(self, method: str, send_body: bool) -> None:
        parsed = urlparse(self.path)
        path = self._normalize(parsed.path)
        query = parse_qs(parsed.query)

        if path.startswith("/api/"):
            self._dispatch_api(method, path, query, send_body)
            return

        if method not in ("GET", "HEAD"):
            self._send_json_status(
                405,
                {"status": "error", "error": "method_not_allowed"},
                extra_headers=TEXT_HEADERS,
            )
            return

        if path == "/health":
            policy_status = self.server.policy.public_status()
            self._send_json(
                {
                    "status": "ok",
                    "service": SERVICE_NAME,
                    "llm": LLM_KIND,
                    "admin_auth": admin_auth_mode(),
                    "kb_loaded": policy_status["kb_loaded"],
                    "harness": HARNESS_KIND,
                },
                send_body=send_body,
            )
            return
        if path in ("/", "/index.html"):
            self._send_static("index.html", "text/html; charset=utf-8", send_body)
            return
        if path in ("/admin", "/admin/"):
            self._send_static("admin.html", "text/html; charset=utf-8", send_body)
            return
        if path in ("/demo", "/demo/"):
            self._send_static("demo.html", "text/html; charset=utf-8", send_body)
            return
        if path in ("/demo-app", "/demo-app/"):
            self._send_static("demo-app.html", "text/html; charset=utf-8", send_body)
            return
        if path == "/embed.js":
            self._send_static(
                "embed.js",
                "application/javascript; charset=utf-8",
                send_body,
                extra_headers=PUBLIC_JS_HEADERS,
            )
            return
        if path == "/embed.css":
            self._send_static(
                "embed.css",
                "text/css; charset=utf-8",
                send_body,
                extra_headers=PUBLIC_CSS_HEADERS,
            )
            return
        self._send_not_found(send_body=send_body)

    def _dispatch_api(
        self, method: str, path: str, query: dict, send_body: bool
    ) -> None:
        if path == "/api/admin/status":
            if method not in ("GET", "HEAD"):
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            policy_status = self.server.policy.public_status()
            self._send_json(
                {
                    "auth": admin_auth_mode(),
                    "service": SERVICE_NAME,
                    "kb_loaded": policy_status["kb_loaded"],
                },
                send_body=send_body,
            )
            return

        if path.startswith("/api/admin/"):
            self._dispatch_admin(method, path, query, send_body)
            return

        if path.startswith("/api/demo-app"):
            self._dispatch_demo_app(method, path, query, send_body)
            return

        if path == "/api/sessions":
            if method == "POST":
                self._create_session()
                return
            self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
            return

        session_match = re.match(
            r"^/api/sessions/("
            + SESSION_ID_RE.pattern[1:-1]
            + r")(/(messages|escalate|tools))?$",
            path,
        )
        if session_match:
            session_id = session_match.group(1)
            action = session_match.group(3)
            if action == "messages":
                if method == "POST":
                    self._post_message(session_id)
                    return
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            if action == "escalate":
                if method == "POST":
                    self._visitor_escalate(session_id)
                    return
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            if action == "tools":
                if method == "POST":
                    self._visitor_tool(session_id)
                    return
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            if method in ("GET", "HEAD"):
                self._get_session(session_id, send_body)
                return
            self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
            return

        self._send_json_status(404, {"status": "not_found", "service": SERVICE_NAME})

    def _dispatch_admin(
        self, method: str, path: str, query: dict, send_body: bool
    ) -> None:
        if method not in ("GET", "HEAD", "POST"):
            self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
            return
        if not self._admin_authorized():
            self._send_json_status(
                401,
                {
                    "status": "unauthorized",
                    "error": "admin_token_required",
                    "message": "Jeton admin manquant ou invalide.",
                },
            )
            return

        if path == "/api/admin/capabilities":
            if method not in ("GET", "HEAD"):
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            self._send_json(
                {
                    "capabilities": capability_catalog(self.server.policy),
                    "default_capabilities": list(self.server.policy.default_capabilities),
                    "declared_tools": self.server.policy.declared_tools_public(),
                    "allowed_origins": list(self.server.policy.allowed_origins),
                    "harness": HARNESS_KIND,
                    "auth": admin_auth_mode(),
                },
                send_body=send_body,
            )
            return

        if path == "/api/admin/journal":
            if method not in ("GET", "HEAD"):
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            session_raw = query.get("session_id", [None])[0]
            session_id = None
            if session_raw:
                if not SESSION_ID_RE.match(session_raw):
                    self._send_json_status(
                        400,
                        {"status": "error", "error": "bad_request", "message": "session_id invalide"},
                    )
                    return
                session_id = session_raw
            entries = self.server.journal.list_entries(session_id=session_id)
            self._send_json(
                {
                    "journal": entries,
                    "harness": HARNESS_KIND,
                    "auth": admin_auth_mode(),
                },
                send_body=send_body,
            )
            return

        site_caps = re.match(
            r"^/api/admin/sites/(" + SITE_ID_RE.pattern[1:-1] + r")/capabilities$",
            path,
        )
        if site_caps:
            site_id = site_caps.group(1)
            if method in ("GET", "HEAD"):
                caps = self.server.policy.capabilities_for(site_id)
                self._send_json(
                    {
                        "site_id": site_id,
                        "capabilities": caps,
                        "kb_available": self.server.policy.kb_present(site_id),
                    },
                    send_body=send_body,
                )
                return
            if method == "POST":
                self._admin_site_capabilities(site_id)
                return

        if path == "/api/admin/sessions":
            if method not in ("GET", "HEAD"):
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            site_raw = query.get("site_id", [None])[0]
            status_raw = query.get("status", [None])[0]
            site_id = None
            if site_raw:
                try:
                    site_id = normalize_site_id(site_raw)
                except ValueError:
                    self._send_json_status(
                        400,
                        {"status": "error", "error": "bad_request", "message": "site_id invalide"},
                    )
                    return
            status = None
            if status_raw:
                if status_raw not in VALID_STATUSES:
                    self._send_json_status(
                        400,
                        {"status": "error", "error": "bad_request", "message": "status invalide"},
                    )
                    return
                status = status_raw
            rows = self.server.store.list_sessions(site_id=site_id, status=status)
            self._send_json({"sessions": rows, "auth": admin_auth_mode()}, send_body=send_body)
            return

        detail_match = re.match(
            r"^/api/admin/sessions/("
            + SESSION_ID_RE.pattern[1:-1]
            + r")(/(capabilities|takeover|messages))?$",
            path,
        )
        if detail_match:
            session_id = detail_match.group(1)
            action = detail_match.group(3)
            if action == "capabilities":
                if method == "POST":
                    self._admin_session_capabilities(session_id)
                    return
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            if action == "takeover":
                if method == "POST":
                    self._admin_takeover(session_id)
                    return
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            if action == "messages":
                if method == "POST":
                    self._admin_human_message(session_id)
                    return
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            if method not in ("GET", "HEAD"):
                self._send_json_status(405, {"status": "error", "error": "method_not_allowed"})
                return
            session = self.server.store.get(
                session_id, include_messages=True, admin=True
            )
            if session is None:
                self._send_json_status(
                    404, {"status": "not_found", "error": "unknown_session"}
                )
                return
            self._send_json(session, send_body=send_body)
            return
        self._send_json_status(404, {"status": "not_found", "service": SERVICE_NAME})

    def _create_session(self) -> None:
        payload, error = self._read_json_object()
        if error is not None:
            self._send_json_status(error[0], error[1], extra_headers=VISITOR_API_HEADERS)
            return
        try:
            site_id = normalize_site_id(payload.get("site_id"))
        except ValueError:
            self._send_json_status(
                400,
                {"status": "error", "error": "bad_request", "message": "site_id invalide"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        try:
            session = self.server.store.create(site_id)
        except OverflowError:
            self._send_json_status(
                503,
                {"status": "error", "error": "too_many_sessions"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        session["llm"] = LLM_KIND
        self._send_json(session, send_body=True, status=201, extra_headers=VISITOR_API_HEADERS)

    def _get_session(self, session_id: str, send_body: bool) -> None:
        session = self.server.store.get(session_id, include_messages=True, admin=False)
        if session is None:
            self._send_json_status(
                404,
                {"status": "not_found", "error": "unknown_session"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        session["llm"] = LLM_KIND
        self._send_json(session, send_body=send_body, extra_headers=VISITOR_API_HEADERS)

    def _post_message(self, session_id: str) -> None:
        payload, error = self._read_json_object()
        if error is not None:
            self._send_json_status(error[0], error[1], extra_headers=VISITOR_API_HEADERS)
            return
        content, cerr = self._require_content(payload)
        if cerr is not None:
            self._send_json_status(cerr[0], cerr[1], extra_headers=VISITOR_API_HEADERS)
            return
        try:
            result = self.server.store.add_visitor_message(session_id, content)
        except KeyError:
            self._send_json_status(
                404,
                {"status": "not_found", "error": "unknown_session"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except OverflowError:
            self._send_json_status(
                409,
                {"status": "error", "error": "too_many_messages"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except SessionConflict as exc:
            self._send_json_status(
                409,
                {"status": "error", "error": exc.code},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        self._send_json(result, send_body=True, status=201, extra_headers=VISITOR_API_HEADERS)

    def _visitor_escalate(self, session_id: str) -> None:
        payload, error = self._read_json_object()
        if error is not None:
            self._send_json_status(error[0], error[1], extra_headers=VISITOR_API_HEADERS)
            return
        reason = ""
        if payload:
            raw = payload.get("reason") or payload.get("content") or ""
            if isinstance(raw, str):
                reason = raw.strip()[:MAX_CONTENT_CHARS]
        try:
            session = self.server.store.escalate(
                session_id, reason=reason, actor="visitor"
            )
        except KeyError:
            self._send_json_status(
                404,
                {"status": "not_found", "error": "unknown_session"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except CapabilityDenied as exc:
            self._send_denied(exc, extra_headers=VISITOR_API_HEADERS)
            return
        except SessionConflict as exc:
            self._send_json_status(
                409,
                {"status": "error", "error": exc.code},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except OverflowError:
            self._send_json_status(
                409,
                {"status": "error", "error": "too_many_messages"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        public = self.server.store.get(session_id, include_messages=True, admin=False)
        self._send_json(
            public or session, send_body=True, status=200, extra_headers=VISITOR_API_HEADERS
        )

    def _visitor_tool(self, session_id: str) -> None:
        payload, error = self._read_json_object()
        if error is not None:
            self._send_json_status(error[0], error[1], extra_headers=VISITOR_API_HEADERS)
            return
        tool = payload.get("tool") if payload else None
        try:
            validate_capability_name(tool)
        except ValueError:
            self._send_json_status(
                400,
                {"status": "error", "error": "bad_request", "message": "tool invalide"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        origin = ""
        raw_origin = payload.get("origin") if payload else None
        if isinstance(raw_origin, str):
            origin = raw_origin.strip()
        args = payload.get("args") if payload else None
        if args is not None and not isinstance(args, dict):
            self._send_json_status(
                400,
                {"status": "error", "error": "bad_request", "message": "args objet requis"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        try:
            result = self.server.store.invoke_tool(
                session_id,
                str(tool),
                origin=origin,
                args=args if isinstance(args, dict) else {},
                self_origin=self._self_origin(),
            )
        except KeyError:
            self._send_json_status(
                404,
                {"status": "not_found", "error": "unknown_session"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except CapabilityDenied as exc:
            public = self.server.store.get(session_id, include_messages=False, admin=False)
            self._send_json(
                {
                    "status": "error",
                    "error": "capability_denied",
                    "capability": exc.capability,
                    "tool": exc.tool or exc.capability,
                    "request_id": exc.request_id,
                    "message": (
                        "Outil refuse : absent de l'allowlist. "
                        "Aucun acte execute. Vous pouvez demander un humain."
                    ),
                    "escalate": True,
                    "session_id": session_id,
                    "session_status": (public or {}).get("status"),
                },
                send_body=True,
                status=403,
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except agent_tools.OriginDenied as exc:
            public = self.server.store.get(session_id, include_messages=False, admin=False)
            self._send_json(
                {
                    "status": "error",
                    "error": "origin_denied",
                    "origin": exc.origin,
                    "request_id": exc.request_id,
                    "tool": str(tool),
                    "message": "Origine hors allowlist. Aucun acte execute.",
                    "escalate": True,
                    "session_id": session_id,
                    "session_status": (public or {}).get("status"),
                    "harness": HARNESS_KIND,
                },
                send_body=True,
                status=403,
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except agent_tools.ToolUndeclared as exc:
            public = self.server.store.get(session_id, include_messages=False, admin=False)
            self._send_json(
                {
                    "status": "error",
                    "error": "tool_undeclared",
                    "tool": exc.tool,
                    "request_id": exc.request_id,
                    "message": (
                        "Outil absent de la config operateur. "
                        "Vous pouvez demander un humain."
                    ),
                    "escalate": True,
                    "session_id": session_id,
                    "session_status": (public or {}).get("status"),
                },
                send_body=True,
                status=403,
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except agent_tools.ToolArgsError as exc:
            self._send_json(
                {
                    "status": "error",
                    "error": "bad_request",
                    "message": str(exc),
                    "request_id": exc.request_id,
                    "tool": str(tool),
                },
                send_body=True,
                status=400,
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except SessionConflict as exc:
            code = 501 if exc.code == "tool_not_implemented" else 409
            self._send_json_status(
                code,
                {"status": "error", "error": exc.code, "tool": str(tool)},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        except OverflowError:
            self._send_json_status(
                409,
                {"status": "error", "error": "too_many_messages"},
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        self._send_json(result, send_body=True, status=200, extra_headers=VISITOR_API_HEADERS)

    def _self_origin(self) -> str:
        host = (self.headers.get("Host") or "").strip()
        if not host:
            address = self.server.server_address
            host = "%s:%s" % (address[0], address[1])
        if host.startswith("http://") or host.startswith("https://"):
            return agent_tools.normalize_origin(host)
        return agent_tools.normalize_origin("http://%s" % host)

    def _dispatch_demo_app(
        self, method: str, path: str, query: dict, send_body: bool
    ) -> None:
        if path == "/api/demo-app/state":
            if method not in ("GET", "HEAD"):
                self._send_json_status(
                    405,
                    {"status": "error", "error": "method_not_allowed"},
                    extra_headers=VISITOR_API_HEADERS,
                )
                return
            self._send_json(
                self.server.harness.snapshot(),
                send_body=send_body,
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        if path == "/api/demo-app/invoices":
            if method not in ("GET", "HEAD"):
                self._send_json_status(
                    405,
                    {"status": "error", "error": "method_not_allowed"},
                    extra_headers=VISITOR_API_HEADERS,
                )
                return
            session_raw = query.get("session_id", [None])[0]
            session_id = None
            if session_raw:
                if not SESSION_ID_RE.match(session_raw):
                    self._send_json_status(
                        400,
                        {"status": "error", "error": "bad_request", "message": "session_id invalide"},
                        extra_headers=VISITOR_API_HEADERS,
                    )
                    return
                session_id = session_raw
            rows = self.server.invoices.list_invoices(session_id=session_id)
            self._send_json(
                {"invoices": rows, "harness": HARNESS_KIND},
                send_body=send_body,
                extra_headers=VISITOR_API_HEADERS,
            )
            return
        self._send_json_status(
            404,
            {"status": "not_found", "service": SERVICE_NAME},
            extra_headers=VISITOR_API_HEADERS,
        )

    def _admin_session_capabilities(self, session_id: str) -> None:
        payload, error = self._read_json_object()
        if error is not None:
            self._send_json_status(error[0], error[1])
            return
        try:
            grant, revoke, replace = self._parse_cap_patch(payload)
        except ValueError as exc:
            self._send_json_status(
                400,
                {"status": "error", "error": "bad_request", "message": str(exc)},
            )
            return
        try:
            result = self.server.store.set_capabilities(
                session_id, grant=grant, revoke=revoke, replace=replace
            )
        except KeyError:
            self._send_json_status(404, {"status": "not_found", "error": "unknown_session"})
            return
        self._send_json(result, send_body=True, status=200)

    def _admin_site_capabilities(self, site_id: str) -> None:
        payload, error = self._read_json_object()
        if error is not None:
            self._send_json_status(error[0], error[1])
            return
        try:
            grant, revoke, replace = self._parse_cap_patch(payload)
        except ValueError as exc:
            self._send_json_status(
                400,
                {"status": "error", "error": "bad_request", "message": str(exc)},
            )
            return
        if replace is not None:
            caps = self.server.policy.set_site_capabilities(site_id, replace)
        else:
            caps = self.server.policy.capabilities_for(site_id)
            if grant:
                caps = self.server.policy.grant_site(site_id, grant)
            if revoke:
                caps = self.server.policy.revoke_site(site_id, revoke)
        self._send_json(
            {"site_id": site_id, "capabilities": caps},
            send_body=True,
            status=200,
        )

    def _admin_takeover(self, session_id: str) -> None:
        _, error = self._read_json_object()
        if error is not None:
            self._send_json_status(error[0], error[1])
            return
        try:
            result = self.server.store.takeover(session_id)
        except KeyError:
            self._send_json_status(404, {"status": "not_found", "error": "unknown_session"})
            return
        except CapabilityDenied as exc:
            self._send_denied(exc)
            return
        except SessionConflict as exc:
            self._send_json_status(409, {"status": "error", "error": exc.code})
            return
        except OverflowError:
            self._send_json_status(409, {"status": "error", "error": "too_many_messages"})
            return
        self._send_json(result, send_body=True, status=200)

    def _admin_human_message(self, session_id: str) -> None:
        payload, error = self._read_json_object()
        if error is not None:
            self._send_json_status(error[0], error[1])
            return
        content, cerr = self._require_content(payload)
        if cerr is not None:
            self._send_json_status(cerr[0], cerr[1])
            return
        try:
            result = self.server.store.add_human_message(session_id, content)
        except KeyError:
            self._send_json_status(404, {"status": "not_found", "error": "unknown_session"})
            return
        except CapabilityDenied as exc:
            self._send_denied(exc)
            return
        except SessionConflict as exc:
            self._send_json_status(409, {"status": "error", "error": exc.code})
            return
        except OverflowError:
            self._send_json_status(409, {"status": "error", "error": "too_many_messages"})
            return
        self._send_json(result, send_body=True, status=201)

    def _parse_cap_patch(
        self, payload: dict
    ) -> tuple[Optional[list[str]], Optional[list[str]], Optional[list[str]]]:
        grant = payload.get("grant")
        revoke = payload.get("revoke")
        replace = payload.get("set")
        if payload.get("capabilities") is not None and replace is None:
            replace = payload.get("capabilities")

        def parse_list(raw: Any, label: str) -> Optional[list[str]]:
            if raw is None:
                return None
            if not isinstance(raw, list):
                raise ValueError("%s doit etre une liste" % label)
            return [validate_capability_name(item) for item in raw]

        return parse_list(grant, "grant"), parse_list(revoke, "revoke"), parse_list(
            replace, "set"
        )

    def _require_content(self, payload: dict) -> tuple[str, Optional[tuple[int, dict]]]:
        content = payload.get("content") if payload else None
        if not isinstance(content, str):
            return "", (
                400,
                {"status": "error", "error": "bad_request", "message": "content texte requis"},
            )
        content = content.strip()
        if not content:
            return "", (
                400,
                {"status": "error", "error": "bad_request", "message": "content vide"},
            )
        if len(content) > MAX_CONTENT_CHARS:
            return "", (
                400,
                {"status": "error", "error": "bad_request", "message": "content trop long"},
            )
        return content, None

    def _send_denied(
        self, exc: CapabilityDenied, extra_headers: Optional[dict] = None
    ) -> None:
        self._send_json(
            {
                "status": "error",
                "error": "capability_denied",
                "capability": exc.capability,
                "tool": exc.tool or exc.capability,
                "request_id": getattr(exc, "request_id", "") or "",
                "message": "Droit absent ou revoque.",
            },
            send_body=True,
            status=403,
            extra_headers=extra_headers,
        )

    def _read_json_object(self) -> tuple[dict, Optional[tuple[int, dict]]]:
        length_raw = self.headers.get("Content-Length", "0") or "0"
        try:
            length = int(length_raw)
        except ValueError:
            return {}, (400, {"status": "error", "error": "bad_request", "message": "Content-Length invalide"})
        if length < 0 or length > MAX_BODY_BYTES:
            return {}, (413, {"status": "error", "error": "payload_too_large"})
        raw = self.rfile.read(length) if length else b""
        if not raw:
            return {}, None
        try:
            payload = json.loads(raw.decode("utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            return {}, (400, {"status": "error", "error": "bad_request", "message": "JSON invalide"})
        if not isinstance(payload, dict):
            return {}, (400, {"status": "error", "error": "bad_request", "message": "objet JSON requis"})
        return payload, None

    def _extract_admin_token(self) -> str:
        header = self.headers.get("Authorization") or ""
        prefix = "Bearer "
        if header.startswith(prefix):
            return header[len(prefix) :].strip()
        return (self.headers.get("X-Admin-Token") or "").strip()

    def _admin_authorized(self) -> bool:
        expected = env_admin_token()
        if not expected:
            return True
        provided = self._extract_admin_token()
        if not provided or len(provided) != len(expected):
            return False
        return hmac.compare_digest(provided, expected)

    @staticmethod
    def _normalize(path: str) -> str:
        decoded = unquote(path)
        collapsed = posixpath.normpath(decoded)
        if not collapsed.startswith("/"):
            collapsed = "/" + collapsed
        if collapsed != "/" and collapsed.endswith("/"):
            return collapsed.rstrip("/") or "/"
        return collapsed

    def _send_json(
        self,
        payload: dict,
        send_body: bool,
        status: int = 200,
        extra_headers: Optional[dict] = None,
    ) -> None:
        raw = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        self.send_response(status)
        headers = extra_headers or TEXT_HEADERS
        self._write_headers(headers)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        if send_body:
            self.wfile.write(raw)

    def _send_json_status(
        self, status: int, payload: dict, extra_headers: Optional[dict] = None
    ) -> None:
        self._send_json(payload, send_body=True, status=status, extra_headers=extra_headers)

    def _send_static(
        self,
        name: str,
        content_type: str,
        send_body: bool,
        extra_headers: Optional[dict] = None,
    ) -> None:
        path = safe_static_path(name)
        if path is None:
            self._send_not_found(send_body=send_body)
            return
        data = path.read_bytes()
        self.send_response(200)
        headers = extra_headers or TEXT_HEADERS
        self._write_headers(headers)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        if send_body:
            self.wfile.write(data)

    def _send_not_found(self, send_body: bool) -> None:
        body = b'{"status":"not_found","service":"mohhdy-agent"}'
        self.send_response(404)
        self._write_headers(TEXT_HEADERS)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        if send_body:
            self.wfile.write(body)

    def _write_headers(self, headers: dict) -> None:
        for key, value in headers.items():
            self.send_header(key, value)


def make_server(
    host: Optional[str] = None, port: Optional[int] = None
) -> AgentHTTPServer:
    bind_host = env_host() if host is None else host
    bind_port = env_port() if port is None else port
    httpd = AgentHTTPServer((bind_host, bind_port), AgentHandler)
    httpd.daemon_threads = True
    return httpd


def main() -> None:
    if not STATIC_DIR.is_dir():
        raise SystemExit("repertoire static/ introuvable: %s" % STATIC_DIR)

    httpd = make_server()
    host, port = httpd.server_address[:2]
    policy_status = httpd.policy.public_status()
    sys.stderr.write(
        "mohhdy-agent ASSIST-020/021/022 sur http://%s:%s "
        "(health=/health admin=/admin embed=/embed.js demo=/demo "
        "demo-app=/demo-app llm=%s harness=%s admin_auth=%s kb_loaded=%s)\n"
        % (
            host,
            port,
            LLM_KIND,
            HARNESS_KIND,
            admin_auth_mode(),
            "oui" if policy_status["kb_loaded"] else "non",
        )
    )
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        sys.stderr.write("mohhdy-agent: arret\n")
    finally:
        httpd.server_close()


if __name__ == "__main__":
    main()
