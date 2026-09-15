#!/usr/bin/env python3
"""Sandbox FS navigateur (ASSIST-061). Listage et lecture bornes.

Racines virtuelles : demo/ (assets statiques) et data/ (MOHHDY_AGENT_DATA).
Pas le FS du guest AOS. Pas US-031. Pas d'Internet public.
Les ecritures ne sont pas exposees (tranche lecture).
"""

from __future__ import annotations

import os
import posixpath
import stat
from pathlib import Path
from typing import Any, Optional

VIRTUAL_DEMO = "demo"
VIRTUAL_DATA = "data"
MAX_PATH_CHARS = 256
MAX_LIST_ENTRIES = 200
MAX_READ_BYTES = 65536
MAX_NAME_CHARS = 128

ALLOWED_READ_SUFFIXES = (
    ".html",
    ".htm",
    ".css",
    ".js",
    ".json",
    ".txt",
    ".md",
    ".svg",
    ".csv",
    ".xml",
)

TEXT_NAME_ALLOWLIST = frozenset(
    {
        "LICENSE",
        "README",
        "NOTICE",
        "CHANGELOG",
    }
)


class FsError(Exception):
    def __init__(self, code: str, message: str = "", http_status: int = 400) -> None:
        super().__init__(code)
        self.code = code
        self.message = message or code
        self.http_status = http_status


def normalize_runtime(raw: Any) -> Optional[str]:
    """docker | browser. Toute autre valeur est ignoree."""
    if raw is None:
        return None
    text = str(raw).strip().lower().replace("-", "_")
    if not text:
        return None
    if text in ("docker", "container", "agent_docker"):
        return "docker"
    if text in ("browser", "browser_runtime", "direct_browser", "web"):
        return "browser"
    return None


def env_runtime() -> Optional[str]:
    return normalize_runtime(os.environ.get("MOHHDY_AGENT_RUNTIME"))


def _is_hidden_name(name: str) -> bool:
    return name.startswith(".") or name in (".", "..")


def _sanitize_virtual_path(raw: Any) -> str:
    if raw is None:
        return ""
    if not isinstance(raw, str):
        raise FsError("path_denied", "chemin invalide", 403)
    text = raw.replace("\\", "/").strip()
    if not text or text == "/":
        return ""
    if "\x00" in text:
        raise FsError("path_denied", "chemin invalide", 403)
    if len(text) > MAX_PATH_CHARS:
        raise FsError("path_denied", "chemin trop long", 403)
    if text.startswith("/"):
        text = text.lstrip("/")
    collapsed = posixpath.normpath(text)
    if collapsed == ".":
        return ""
    if collapsed.startswith("..") or "/../" in ("/" + collapsed + "/"):
        raise FsError("path_denied", "traversal refuse", 403)
    if collapsed.startswith("/"):
        raise FsError("path_denied", "chemin absolu refuse", 403)
    parts = [part for part in collapsed.split("/") if part]
    if not parts:
        return ""
    for part in parts:
        if _is_hidden_name(part):
            raise FsError("path_denied", "fichier cache refuse", 403)
        if part in (".", ".."):
            raise FsError("path_denied", "traversal refuse", 403)
        if len(part) > MAX_NAME_CHARS:
            raise FsError("path_denied", "nom trop long", 403)
    return "/".join(parts)


def _readable_file(name: str) -> bool:
    if _is_hidden_name(name):
        return False
    lowered = name.lower()
    if lowered.endswith((".pem", ".key", ".crt", ".p12", ".env")):
        return False
    if lowered in (".env", ".git"):
        return False
    suffix = Path(name).suffix.lower()
    if suffix in ALLOWED_READ_SUFFIXES:
        return True
    stem = Path(name).stem.upper()
    return stem in TEXT_NAME_ALLOWLIST and suffix == ""


class BrowserSandbox:
    """FS virtuel: demo assets + donnees d'instance, sans sortir du bac a sable."""

    def __init__(
        self,
        demo_root: Path,
        data_root: Optional[Path] = None,
    ) -> None:
        self.demo_root = demo_root
        self.data_root = data_root

    def public_roots(self) -> list[dict[str, Any]]:
        rows = [
            {
                "name": VIRTUAL_DEMO,
                "kind": "demo_assets",
                "path": VIRTUAL_DEMO,
                "writable": False,
                "description": "Pages et assets du runtime (static/), hors code Python",
            }
        ]
        if self._data_available():
            rows.append(
                {
                    "name": VIRTUAL_DATA,
                    "kind": "instance_data",
                    "path": VIRTUAL_DATA,
                    "writable": False,
                    "description": "Repertoire MOHHDY_AGENT_DATA (sessions, factures demo)",
                }
            )
        return rows

    def inspect(self, raw_path: Any, op: Optional[str] = None) -> dict[str, Any]:
        virtual = _sanitize_virtual_path(raw_path)
        want = (op or "").strip().lower()
        if not virtual:
            if want == "read":
                raise FsError("is_directory", "la racine n'est pas un fichier", 400)
            return {
                "path": "",
                "type": "roots",
                "entries": self.public_roots(),
            }
        real, virtual_full, is_dir = self._resolve(virtual)
        if want == "read" or (want != "list" and not is_dir):
            if is_dir:
                raise FsError("is_directory", "ce chemin est un repertoire", 400)
            return self._read_resolved(real, virtual_full)
        if not is_dir:
            raise FsError("not_a_directory", "ce chemin n'est pas un repertoire", 400)
        return self._list_resolved(real, virtual_full)

    def _data_available(self) -> bool:
        root = self.data_root
        if root is None:
            return False
        try:
            return root.is_dir()
        except OSError:
            return False

    def _root_map(self) -> dict[str, Path]:
        mapping = {VIRTUAL_DEMO: self.demo_root}
        if self._data_available() and self.data_root is not None:
            mapping[VIRTUAL_DATA] = self.data_root
        return mapping

    def _resolve(self, virtual: str) -> tuple[Path, str, bool]:
        parts = virtual.split("/")
        root_name = parts[0]
        mapping = self._root_map()
        if root_name not in mapping:
            raise FsError("path_denied", "racine inconnue", 403)
        root = mapping[root_name]
        try:
            root_real = root.resolve(strict=False)
        except OSError as exc:
            raise FsError("path_denied", "racine illisible", 403) from exc
        if not root_real.is_dir():
            raise FsError("path_denied", "racine absente", 404)

        rel_parts = parts[1:]
        candidate = root_real
        for part in rel_parts:
            candidate = candidate / part
        try:
            real = candidate.resolve(strict=False)
        except OSError as exc:
            raise FsError("path_denied", "chemin illisible", 403) from exc

        try:
            real.relative_to(root_real)
        except ValueError as exc:
            raise FsError("path_denied", "traversal refuse", 403) from exc

        if not real.exists():
            raise FsError("not_found", "introuvable", 404)

        # Refuse un lien qui pointerait hors du bac (resolve a deja suivi).
        if real.is_symlink():
            try:
                target = real.resolve(strict=True)
                target.relative_to(root_real)
            except (OSError, ValueError) as exc:
                raise FsError("path_denied", "lien hors sandbox", 403) from exc

        is_dir = real.is_dir()
        virtual_full = "/".join(parts)
        return real, virtual_full, is_dir

    def _list_resolved(self, real: Path, virtual_full: str) -> dict[str, Any]:
        entries: list[dict[str, Any]] = []
        try:
            names = sorted(os.listdir(real))
        except OSError as exc:
            raise FsError("path_denied", "repertoire illisible", 403) from exc
        root_real = self._root_real_for(virtual_full)
        for name in names:
            if len(entries) >= MAX_LIST_ENTRIES:
                break
            if _is_hidden_name(name):
                continue
            child = real / name
            try:
                st = child.lstat()
            except OSError:
                continue
            if stat.S_ISLNK(st.st_mode):
                try:
                    target = child.resolve(strict=True)
                    target.relative_to(root_real)
                except (OSError, ValueError, FsError):
                    continue
                kind = "dir" if target.is_dir() else "file"
                size = target.stat().st_size if kind == "file" else None
            elif stat.S_ISDIR(st.st_mode):
                kind = "dir"
                size = None
            elif stat.S_ISREG(st.st_mode):
                kind = "file"
                size = st.st_size
            else:
                continue
            if kind == "file" and not _readable_file(name):
                continue
            child_path = virtual_full + "/" + name
            row: dict[str, Any] = {
                "name": name,
                "type": kind,
                "path": child_path,
            }
            if size is not None:
                row["size"] = size
            entries.append(row)
        return {
            "path": virtual_full,
            "type": "dir",
            "entries": entries,
            "truncated": len(entries) >= MAX_LIST_ENTRIES,
        }

    def _root_of(self, virtual_full: str) -> Path:
        root_name = virtual_full.split("/", 1)[0]
        mapping = self._root_map()
        if root_name not in mapping:
            raise FsError("path_denied", "racine inconnue", 403)
        return mapping[root_name].resolve(strict=False)

    def _root_real_for(self, virtual_full: str) -> Path:
        return self._root_of(virtual_full)

    def _read_resolved(self, real: Path, virtual_full: str) -> dict[str, Any]:
        name = real.name
        if not _readable_file(name):
            raise FsError("not_text", "type de fichier refuse", 415)
        if not real.is_file():
            raise FsError("not_found", "pas un fichier", 404)
        try:
            data = real.read_bytes()
        except OSError as exc:
            raise FsError("path_denied", "fichier illisible", 403) from exc
        truncated = len(data) > MAX_READ_BYTES
        if truncated:
            data = data[:MAX_READ_BYTES]
        try:
            text = data.decode("utf-8")
            encoding = "utf-8"
        except UnicodeDecodeError:
            text = data.decode("utf-8", errors="replace")
            encoding = "utf-8-replace"
        return {
            "path": virtual_full,
            "type": "file",
            "name": name,
            "size": real.stat().st_size,
            "truncated": truncated,
            "encoding": encoding,
            "content": text,
        }
