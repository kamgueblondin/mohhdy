#!/usr/bin/env python3
"""Moteur navigateur optionnel (Playwright / Chromium), hors chemin slim.

Import de ce module : stdlib seulement. Playwright n'est charge que si
MOHHDY_AGENT_BROWSER_ENGINE=playwright|chromium ET si le paquet est
present. Defaut : optional_not_installed. Les gestes de session restent
le simulateur DOM (tools.py). Ce n'est pas US-031.
"""

from __future__ import annotations

import importlib.util
import os
import threading
from typing import Any, Callable, Optional
from urllib.parse import urljoin, urlparse

import tools as agent_tools

ENGINE_NONE = "optional_not_installed"
ENGINE_PLAYWRIGHT = "playwright"
ENGINE_CHROMIUM = "chromium"
VALID_ENGINE_NAMES = (ENGINE_PLAYWRIGHT, ENGINE_CHROMIUM)
SESSION_TOOLS_HARNESS = "dom_simulator"
MAX_URL_CHARS = 2048
NAVIGATE_TIMEOUT_MS = 8000
SCREENSHOT_TIMEOUT_MS = 5000

ENGINE_NOTE_OFF = (
    "profil optionnel, non installe ; les gestes de session restent "
    "le simulateur DOM ; ce n'est pas US-031"
)
ENGINE_NOTE_ON = (
    "controle operateur /browser seulement ; les gestes de session "
    "restent le simulateur DOM ; ce n'est pas US-031"
)


class EngineUnavailable(RuntimeError):
    def __init__(self, message: str = ENGINE_NOTE_OFF) -> None:
        super().__init__(message)
        self.code = ENGINE_NONE


class UrlDenied(PermissionError):
    def __init__(self, url: str, origin: str) -> None:
        super().__init__("origin_denied")
        self.url = url
        self.origin = origin


class UrlError(ValueError):
    pass


def requested_from_env(raw: Optional[str] = None) -> str:
    if raw is None:
        raw = os.environ.get("MOHHDY_AGENT_BROWSER_ENGINE")
    text = (raw or "").strip().lower().replace("-", "_")
    if text in ("", "off", "none", "default", "dom_simulator", "optional_not_installed"):
        return ""
    if text in ("playwright", "pw"):
        return ENGINE_PLAYWRIGHT
    if text in ("chromium", "chrome"):
        return ENGINE_CHROMIUM
    return ""


def playwright_package_present() -> bool:
    try:
        return importlib.util.find_spec("playwright") is not None
    except (ImportError, ValueError, ModuleNotFoundError):
        return False


def engine_label(requested: Optional[str] = None) -> str:
    req = requested_from_env() if requested is None else requested
    if not req:
        return ENGINE_NONE
    if playwright_package_present():
        return req
    return ENGINE_NONE


def resolve_target_url(raw: str, self_origin: str) -> str:
    text = (raw or "").strip()
    if not text:
        raise UrlError("url requise")
    if len(text) > MAX_URL_CHARS:
        raise UrlError("url trop longue")
    if text.startswith("/") and not text.startswith("//"):
        base = (self_origin or "").rstrip("/")
        if not base:
            raise UrlError("origine locale absente")
        return urljoin(base + "/", text.lstrip("/"))
    parsed = urlparse(text)
    if parsed.scheme not in ("http", "https"):
        raise UrlDenied(text, parsed.scheme or "")
    if not parsed.netloc:
        raise UrlDenied(text, "")
    return text


def origin_of_url(url: str) -> str:
    return agent_tools.normalize_origin(url)


def url_allowed(url: str, allowlist: list[str], self_origin: str) -> bool:
    origin = origin_of_url(url)
    if not origin:
        return False
    return agent_tools.origin_allowed(origin, allowlist, self_origin)


def launch_args() -> list[str]:
    args = ["--disable-dev-shm-usage"]
    raw = (os.environ.get("MOHHDY_AGENT_PLAYWRIGHT_NO_SANDBOX") or "").strip().lower()
    if raw in ("1", "true", "yes", "on"):
        args.append("--no-sandbox")
    return args


def _load_sync_playwright() -> Any:
    from playwright.sync_api import sync_playwright

    return sync_playwright()


class OptionalBrowser:
    """Session Playwright paresseuse, un onglet operateur."""

    def __init__(
        self,
        launcher: Optional[Callable[[], Any]] = None,
        requested: Optional[str] = None,
    ) -> None:
        self._lock = threading.Lock()
        self._launcher = launcher
        self._requested_override = requested
        self._pw: Any = None
        self._browser: Any = None
        self._page: Any = None
        self._screenshot = b""
        self._url = ""
        self._title = ""
        self._last_error = ""

    def requested(self) -> str:
        if self._requested_override is not None:
            return self._requested_override
        return requested_from_env()

    def label(self) -> str:
        req = self.requested()
        if not req:
            return ENGINE_NONE
        if self._launcher is not None or playwright_package_present():
            return req
        return ENGINE_NONE

    def available(self) -> bool:
        return self.label() != ENGINE_NONE

    def page_state(self) -> dict[str, Any]:
        with self._lock:
            return {
                "url": self._url,
                "title": self._title,
                "screenshot_available": bool(self._screenshot),
            }

    def public_status(self) -> dict[str, Any]:
        label = self.label()
        requested = self.requested()
        return {
            "browser_engine": label,
            "browser_engine_requested": requested,
            "session_tools_harness": SESSION_TOOLS_HARNESS,
            "engine_note": ENGINE_NOTE_ON if label != ENGINE_NONE else ENGINE_NOTE_OFF,
            "page": self.page_state(),
            "phase3_complete": False,
            "us031_complete": False,
        }

    def screenshot_bytes(self) -> bytes:
        if not self.available():
            raise EngineUnavailable()
        with self._lock:
            if not self._screenshot:
                raise LookupError("page_not_open")
            return self._screenshot

    def navigate(self, raw_url: str, allowlist: list[str], self_origin: str) -> dict[str, Any]:
        if not self.available():
            raise EngineUnavailable()
        target = resolve_target_url(raw_url, self_origin)
        if not url_allowed(target, allowlist, self_origin):
            raise UrlDenied(target, origin_of_url(target))
        with self._lock:
            page = self._ensure_page_unlocked()
            page.goto(
                target,
                wait_until="domcontentloaded",
                timeout=NAVIGATE_TIMEOUT_MS,
            )
            title = ""
            try:
                title = page.title() or ""
            except Exception:
                title = ""
            shot = b""
            try:
                shot = page.screenshot(type="png", timeout=SCREENSHOT_TIMEOUT_MS) or b""
            except Exception:
                shot = b""
            current = ""
            try:
                current = str(page.url or target)
            except Exception:
                current = target
            self._url = current
            self._title = str(title)[:500]
            self._screenshot = shot if isinstance(shot, (bytes, bytearray)) else b""
            self._last_error = ""
            return {
                "ok": True,
                "url": self._url,
                "title": self._title,
                "screenshot_available": bool(self._screenshot),
                "browser_engine": self.label(),
                "harness": SESSION_TOOLS_HARNESS,
                "phase3_complete": False,
                "us031_complete": False,
            }

    def reset(self) -> None:
        with self._lock:
            self._close_unlocked()

    def close(self) -> None:
        self.reset()

    def _ensure_page_unlocked(self) -> Any:
        if self._page is not None:
            return self._page
        factory = self._launcher or _load_sync_playwright
        try:
            handle = factory()
        except ImportError as exc:
            raise EngineUnavailable("paquet playwright absent") from exc
        started = handle
        if hasattr(handle, "start"):
            started = handle.start()
        self._pw = started
        chromium = getattr(started, "chromium", None)
        if chromium is None:
            raise EngineUnavailable("chromium playwright absent")
        try:
            self._browser = chromium.launch(headless=True, args=launch_args())
        except Exception as exc:
            self._close_unlocked()
            raise EngineUnavailable("lancement chromium impossible") from exc
        self._page = self._browser.new_page()
        return self._page

    def _close_unlocked(self) -> None:
        page = self._page
        browser = self._browser
        pw = self._pw
        self._page = None
        self._browser = None
        self._pw = None
        self._screenshot = b""
        self._url = ""
        self._title = ""
        self._last_error = ""
        for obj, method in ((page, "close"), (browser, "close"), (pw, "stop")):
            if obj is None:
                continue
            closer = getattr(obj, method, None)
            if closer is None:
                continue
            try:
                closer()
            except Exception:
                pass
