#!/usr/bin/env python3
"""Outils de session ASSIST-020/021/022 : simulateur DOM, MCP, factures, journal.

HARNESS : simulateur DOM in-process, pas Chromium / Playwright. Les gestes
mutent un etat observable via /api/demo-app et la page /demo-app (meme
conteneur, sans Internet). Le chemin slim (make agent-smoke) n'installe
aucun navigateur.
"""

from __future__ import annotations

import json
import threading
import uuid
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Optional
from urllib.parse import urlparse

HARNESS_KIND = "dom_simulator"
HARNESS_NOTE = (
    "simulateur DOM in-process, pas un navigateur Chromium/Playwright"
)

CAP_DOM_CLICK = "dom.click"
CAP_DOM_TYPE = "dom.type"
CAP_POINTER = "pointer.move"
CAP_INVOICE = "mcp.invoice.create"

GESTURE_TOOLS = (CAP_DOM_CLICK, CAP_DOM_TYPE, CAP_POINTER)
DEMO_MCP_TOOLS = (CAP_INVOICE,)

ALLOWED_SELECTORS = (
    "#menu-toggle",
    "#menu-invoices",
    "#invoice-customer",
    "#invoice-amount",
    "#invoice-submit",
)

SELECTOR_RE = tuple(ALLOWED_SELECTORS)
MAX_TYPE_CHARS = 200
MAX_JOURNAL = 1000
MAX_INVOICES = 200

DEFAULT_ORIGINS = ["self"]
DEFAULT_DECLARED_TOOLS = {
    CAP_INVOICE: {
        "kind": "mcp",
        "description": "Creer une facture demo dans l'appli hote mock",
    }
}


class OriginDenied(PermissionError):
    def __init__(self, origin: str, request_id: str) -> None:
        super().__init__("origin_denied")
        self.origin = origin
        self.request_id = request_id
        self.capability = "origin"


class ToolUndeclared(PermissionError):
    def __init__(self, tool: str, request_id: str) -> None:
        super().__init__("tool_undeclared")
        self.tool = tool
        self.request_id = request_id


class ToolArgsError(ValueError):
    def __init__(self, message: str, request_id: str = "") -> None:
        super().__init__(message)
        self.request_id = request_id


def utc_now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def is_gesture_tool(name: str) -> bool:
    return name in GESTURE_TOOLS


def is_mcp_tool(name: str) -> bool:
    return name.startswith("mcp.")


def parse_origin(raw: str) -> Optional[tuple[str, str, int, bool]]:
    """(scheme, host, port, port_explicit) ou None."""
    text = (raw or "").strip()
    if not text or text.lower() == "self":
        return None
    if "://" not in text:
        text = "http://" + text
    parsed = urlparse(text)
    if parsed.scheme not in ("http", "https"):
        return None
    host = (parsed.hostname or "").lower()
    if not host:
        return None
    explicit = parsed.port is not None
    if parsed.port:
        port = parsed.port
    else:
        port = 443 if parsed.scheme == "https" else 80
    return parsed.scheme, host, port, explicit


def normalize_origin(raw: str) -> str:
    parsed = parse_origin(raw)
    if parsed is None:
        return ""
    scheme, host, port, _explicit = parsed
    default = 443 if scheme == "https" else 80
    if port == default:
        return "%s://%s" % (scheme, host)
    return "%s://%s:%s" % (scheme, host, port)


def origin_allowed(origin: str, allowlist: list[str], self_origin: str) -> bool:
    target = parse_origin(origin)
    if target is None:
        return False
    self_parsed = parse_origin(self_origin)
    for item in allowlist:
        token = (item or "").strip()
        if not token:
            continue
        if token.lower() == "self":
            if self_parsed and target[:3] == self_parsed[:3]:
                return True
            continue
        candidate = parse_origin(token)
        if candidate is None:
            continue
        if target[0] != candidate[0] or target[1] != candidate[1]:
            continue
        if not candidate[3]:
            return True
        if target[2] == candidate[2]:
            return True
    return False


def parse_origin_list(raw: Any) -> list[str]:
    if not isinstance(raw, list):
        return []
    out: list[str] = []
    for item in raw:
        if not isinstance(item, str):
            continue
        text = item.strip()
        if text:
            out.append(text)
    return out


def parse_declared_tools(raw: Any) -> dict[str, dict[str, str]]:
    declared: dict[str, dict[str, str]] = {}
    if isinstance(raw, dict):
        for name, spec in raw.items():
            if not isinstance(name, str) or not name.startswith("mcp."):
                continue
            description = ""
            kind = "mcp"
            if isinstance(spec, dict):
                description = str(spec.get("description") or "")[:200]
                kind = str(spec.get("kind") or "mcp")[:32]
            elif isinstance(spec, str):
                description = spec[:200]
            declared[name] = {"kind": kind, "description": description}
        return declared
    if isinstance(raw, list):
        for item in raw:
            if isinstance(item, str) and item.startswith("mcp."):
                declared[item] = {"kind": "mcp", "description": ""}
            elif isinstance(item, dict):
                name = str(item.get("name") or "")
                if name.startswith("mcp."):
                    declared[name] = {
                        "kind": str(item.get("kind") or "mcp")[:32],
                        "description": str(item.get("description") or "")[:200],
                    }
    return declared


class DemoHarness:
    """Etat DOM de la page /demo-app. Pas un vrai moteur de rendu."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._state = self._blank()

    @staticmethod
    def _blank() -> dict[str, Any]:
        return {
            "menu_open": False,
            "focused": None,
            "pointer": {"x": 0, "y": 0},
            "form": {"customer": "", "amount": "", "submitted": False},
            "last_request_id": None,
            "last_tool": None,
            "last_selector": None,
            "updated_at": utc_now(),
        }

    def reset(self) -> None:
        with self._lock:
            self._state = self._blank()

    def snapshot(self) -> dict[str, Any]:
        with self._lock:
            return self._copy(self._state)

    def apply_gesture(
        self, tool: str, args: dict[str, Any], request_id: str
    ) -> dict[str, Any]:
        with self._lock:
            if tool == CAP_POINTER:
                x = self._coord(args.get("x"), "x")
                y = self._coord(args.get("y"), "y")
                self._state["pointer"] = {"x": x, "y": y}
                self._state["last_selector"] = None
            elif tool == CAP_DOM_CLICK:
                selector = self._selector(args.get("selector"))
                self._click_unlocked(selector)
                self._state["last_selector"] = selector
            elif tool == CAP_DOM_TYPE:
                selector = self._selector(args.get("selector"))
                text = args.get("text")
                if not isinstance(text, str):
                    raise ToolArgsError("text requis", request_id)
                text = text.strip()
                if not text:
                    raise ToolArgsError("text vide", request_id)
                if len(text) > MAX_TYPE_CHARS:
                    raise ToolArgsError("text trop long", request_id)
                self._type_unlocked(selector, text)
                self._state["last_selector"] = selector
            else:
                raise ToolArgsError("geste inconnu", request_id)
            self._state["last_request_id"] = request_id
            self._state["last_tool"] = tool
            self._state["updated_at"] = utc_now()
            return self._copy(self._state)

    def form_values(self) -> tuple[str, str]:
        with self._lock:
            form = self._state["form"]
            return str(form.get("customer") or ""), str(form.get("amount") or "")

    def _click_unlocked(self, selector: str) -> None:
        if selector == "#menu-toggle":
            self._state["menu_open"] = not bool(self._state["menu_open"])
            self._state["focused"] = selector
            return
        if selector == "#menu-invoices":
            self._state["menu_open"] = True
            self._state["focused"] = "#invoice-customer"
            return
        if selector == "#invoice-customer":
            self._state["focused"] = selector
            return
        if selector == "#invoice-amount":
            self._state["focused"] = selector
            return
        if selector == "#invoice-submit":
            self._state["focused"] = selector
            self._state["form"]["submitted"] = True
            return
        raise ToolArgsError("selecteur non allowliste")

    def _type_unlocked(self, selector: str, text: str) -> None:
        if selector == "#invoice-customer":
            self._state["form"]["customer"] = text
            self._state["focused"] = selector
            return
        if selector == "#invoice-amount":
            self._state["form"]["amount"] = text
            self._state["focused"] = selector
            return
        raise ToolArgsError("selecteur non saisissable")

    @staticmethod
    def _selector(raw: Any) -> str:
        if not isinstance(raw, str):
            raise ToolArgsError("selector requis")
        selector = raw.strip()
        if selector not in SELECTOR_RE:
            raise ToolArgsError("selecteur non allowliste")
        return selector

    @staticmethod
    def _coord(raw: Any, label: str) -> int:
        if isinstance(raw, bool) or raw is None:
            raise ToolArgsError("%s requis" % label)
        try:
            value = int(raw)
        except (TypeError, ValueError) as exc:
            raise ToolArgsError("%s invalide" % label) from exc
        if value < 0:
            value = 0
        if value > 1000:
            value = 1000
        return value

    @staticmethod
    def _copy(state: dict[str, Any]) -> dict[str, Any]:
        return {
            "menu_open": bool(state["menu_open"]),
            "focused": state.get("focused"),
            "pointer": dict(state["pointer"]),
            "form": dict(state["form"]),
            "last_request_id": state.get("last_request_id"),
            "last_tool": state.get("last_tool"),
            "last_selector": state.get("last_selector"),
            "updated_at": state.get("updated_at"),
            "harness": HARNESS_KIND,
        }


class InvoiceStore:
    """Factures demo (memoire, fichier optionnel sous MOHHDY_AGENT_DATA)."""

    def __init__(self, persist_path: Optional[Path] = None) -> None:
        self._lock = threading.Lock()
        self._invoices: dict[str, dict[str, Any]] = {}
        self._persist_path = persist_path
        if persist_path is not None:
            self._load_unlocked()

    def clear(self) -> None:
        with self._lock:
            self._invoices = {}
            self._persist_unlocked()

    def create(
        self,
        session_id: str,
        site_id: str,
        customer: str,
        amount: str,
        request_id: str,
    ) -> dict[str, Any]:
        customer = customer.strip()
        amount = amount.strip()
        if not customer:
            raise ToolArgsError("customer requis", request_id)
        if not amount:
            raise ToolArgsError("amount requis", request_id)
        if len(customer) > 120 or len(amount) > 32:
            raise ToolArgsError("champs facture trop longs", request_id)
        with self._lock:
            if len(self._invoices) >= MAX_INVOICES:
                raise OverflowError("trop de factures")
            invoice_id = "inv-%s" % uuid.uuid4()
            record = {
                "invoice_id": invoice_id,
                "session_id": session_id,
                "site_id": site_id,
                "customer": customer,
                "amount": amount,
                "currency": "EUR",
                "request_id": request_id,
                "created_at": utc_now(),
            }
            self._invoices[invoice_id] = record
            self._persist_unlocked()
            return dict(record)

    def list_invoices(self, session_id: Optional[str] = None) -> list[dict[str, Any]]:
        with self._lock:
            rows = [dict(item) for item in self._invoices.values()]
        if session_id:
            rows = [item for item in rows if item.get("session_id") == session_id]
        rows.sort(key=lambda item: item.get("created_at") or "", reverse=True)
        return rows

    def get(self, invoice_id: str) -> Optional[dict[str, Any]]:
        with self._lock:
            record = self._invoices.get(invoice_id)
            return dict(record) if record else None

    def _load_unlocked(self) -> None:
        path = self._persist_path
        if path is None or not path.is_file():
            return
        try:
            raw = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError, UnicodeDecodeError):
            return
        invoices = raw.get("invoices") if isinstance(raw, dict) else None
        if not isinstance(invoices, dict):
            return
        restored: dict[str, dict[str, Any]] = {}
        for invoice_id, record in invoices.items():
            if isinstance(invoice_id, str) and isinstance(record, dict):
                restored[invoice_id] = record
        self._invoices = restored

    def _persist_unlocked(self) -> None:
        path = self._persist_path
        if path is None:
            return
        payload = json.dumps({"invoices": self._invoices}, separators=(",", ":"))
        tmp = path.with_suffix(".json.tmp")
        try:
            tmp.write_text(payload, encoding="utf-8")
            tmp.replace(path)
        except OSError:
            try:
                tmp.unlink()
            except OSError:
                pass


class AuditJournal:
    """Journal d'actes (request_id, outil, origine, resultat)."""

    def __init__(self, persist_path: Optional[Path] = None) -> None:
        self._lock = threading.Lock()
        self._entries: list[dict[str, Any]] = []
        self._persist_path = persist_path
        if persist_path is not None:
            self._load_unlocked()

    def clear(self) -> None:
        with self._lock:
            self._entries = []
            self._persist_unlocked()

    def record(
        self,
        request_id: str,
        session_id: str,
        site_id: str,
        tool: str,
        origin: str,
        outcome: str,
        extra: Optional[dict[str, Any]] = None,
    ) -> dict[str, Any]:
        entry = {
            "request_id": request_id,
            "session_id": session_id,
            "site_id": site_id,
            "tool": tool,
            "origin": origin,
            "outcome": outcome,
            "harness": HARNESS_KIND,
            "created_at": utc_now(),
        }
        if extra:
            for key, value in extra.items():
                if key in entry:
                    continue
                if isinstance(value, (str, int, float, bool)) or value is None:
                    entry[key] = value
        with self._lock:
            self._entries.append(entry)
            if len(self._entries) > MAX_JOURNAL:
                self._entries = self._entries[-MAX_JOURNAL:]
            self._persist_unlocked()
            return dict(entry)

    def list_entries(
        self,
        session_id: Optional[str] = None,
        limit: int = 100,
    ) -> list[dict[str, Any]]:
        with self._lock:
            rows = list(self._entries)
        if session_id:
            rows = [item for item in rows if item.get("session_id") == session_id]
        rows.reverse()
        if limit < 1:
            limit = 1
        return [dict(item) for item in rows[: min(limit, MAX_JOURNAL)]]

    def _load_unlocked(self) -> None:
        path = self._persist_path
        if path is None or not path.is_file():
            return
        try:
            raw = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError, UnicodeDecodeError):
            return
        entries = raw.get("entries") if isinstance(raw, dict) else None
        if not isinstance(entries, list):
            return
        restored: list[dict[str, Any]] = []
        for item in entries:
            if isinstance(item, dict) and item.get("request_id"):
                restored.append(item)
        self._entries = restored[-MAX_JOURNAL:]

    def _persist_unlocked(self) -> None:
        path = self._persist_path
        if path is None:
            return
        payload = json.dumps({"entries": self._entries}, separators=(",", ":"))
        tmp = path.with_suffix(".json.tmp")
        try:
            tmp.write_text(payload, encoding="utf-8")
            tmp.replace(path)
        except OSError:
            try:
                tmp.unlink()
            except OSError:
                pass


class ToolRunner:
    """Execute un outil deja autorise (gestes simulateur ou MCP demo)."""

    def __init__(self, harness: DemoHarness, invoices: InvoiceStore) -> None:
        self.harness = harness
        self.invoices = invoices

    def execute(
        self,
        tool: str,
        args: dict[str, Any],
        request_id: str,
        session_id: str,
        site_id: str,
    ) -> dict[str, Any]:
        if is_gesture_tool(tool):
            state = self.harness.apply_gesture(tool, args, request_id)
            return {
                "ok": True,
                "harness": HARNESS_KIND,
                "note": HARNESS_NOTE,
                "dom": state,
            }
        if tool == CAP_INVOICE:
            customer = args.get("customer") if isinstance(args.get("customer"), str) else ""
            amount = args.get("amount") if isinstance(args.get("amount"), str) else ""
            form_customer, form_amount = self.harness.form_values()
            if not customer.strip():
                customer = form_customer
            if not amount.strip():
                amount = form_amount
            invoice = self.invoices.create(
                session_id=session_id,
                site_id=site_id,
                customer=str(customer or ""),
                amount=str(amount or ""),
                request_id=request_id,
            )
            return {
                "ok": True,
                "harness": HARNESS_KIND,
                "invoice": invoice,
            }
        raise ToolArgsError("outil non implemente", request_id)
