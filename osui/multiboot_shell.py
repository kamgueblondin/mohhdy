#!/usr/bin/env python3
"""Shell Multiboot du SE (meme vocabulaire que userspace/shell.c Ring 3).

Surface bootstrap osui : les commandes portent les noms du guest
(ai, vfs-list, help, ...). QEMU n'est pas dans ce processus ; l'attache
live (serie / HMP) n'est pas branchee. Ne pas inventer un bash Linux.
"""

from __future__ import annotations

from stage import LLM_KIND, reason_stage

PROMPT = "MOHHDY>"
SOURCE = "userspace/shell.c"
ATTACHMENT_BOOTSTRAP = "bootstrap"
ATTACHMENT_LIVE = "live_guest"

GUEST_COMMANDS = (
    "help",
    "ls",
    "dir",
    "pwd",
    "whoami",
    "sysinfo",
    "info",
    "ps",
    "mem",
    "memory",
    "echo",
    "clear",
    "cls",
    "which",
    "rc",
    "date",
    "uptime",
    "getpid",
    "history",
    "env",
    "ai",
    "ai-help",
    "aihelp",
    "ai-mode",
    "aimode",
    "ai-stats",
    "aistats",
    "ai-provider",
    "ai-runtime",
    "ai-model",
    "ai-continue",
    "ai-test",
    "aitest",
    "vfs-list",
    "vfs-read",
    "vfs-stat",
    "vfs-stats",
    "vfs-write",
    "vfs-remove",
    "vfs-rename",
    "vfs-mkdir",
    "vfs-rmdir",
    "vfs-mount-add",
    "vfs-mount-remove",
    "fat16-list",
    "fat16-cat",
    "net-status",
    "spawn",
    "yield",
    "ipc-send",
    "ipc-recv",
    "service-find",
    "service-status",
    "cat",
    "stat",
    "mkdir",
    "rmdir",
    "rm",
    "touch",
    "write",
    "append",
    "attach",
    "detach",
    "guest-status",
)

LINUX_TRAPS = frozenset(
    {
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
    }
)

OSUI_PANES = ("browser", "shell", "admin", "support", "status", "fs")

HELP_TEXT = """=== MOHHDY Shell (Multiboot Ring 3) ===
Surface : bootstrap osui. Attache guest live : non.
Source guest : userspace/shell.c  prompt guest : MOHHDY>
Pas un bash Linux. Pas un TTY QEMU attache.

COMMANDES SYSTEME :
  ls [path]          - Lister initrd + overlay (miroir bootstrap)
  cat <file>         - Afficher un fichier du miroir
  pwd                - Repertoire courant
  whoami             - Identite shell Multiboot
  sysinfo            - Informations systeme (bootstrap)
  ps                 - Taches (miroir, pas le scheduler guest)
  mem                - Memoire (diagnostic bootstrap)
  which <cmd>        - Builtin guest ou bin/

COMMANDES IA (guest : SYS_GPT2_GENERATE / GGUF) :
  ai <question>      - Stub llm=stub_echo ; met a jour #ai-stage
  ai-help            - Aide IA
  ai-mode [on|off]   - Mode IA local (stub)
  ai-stats           - Stats stub
  ai-provider        - local (openai refuse ici comme au guest)
  ai-runtime         - Etat honnete stub / pas de live QEMU
  ai-continue        - Continuation : non attachee au runtime GGUF

COMMANDES VFS (guest : vfsserver / vfsvirtual) :
  vfs-list <repertoire/>
  vfs-read <chemin>
  vfs-stat <chemin>
  vfs-stats
  vfs-write / vfs-remove / vfs-rename / vfs-mkdir / vfs-rmdir
  vfs-mount-add / vfs-mount-remove
  fat16-list / fat16-cat

RESEAU :
  net-status         - nic=absent (pas de NE2000 dans osui)

ATTACHE :
  guest-status       - bootstrap vs live
  attach             - refuse : QEMU/serial non branche
  detach             - reste bootstrap

OS-UI (hors guest, fenetres du chrome) :
  open browser|shell|admin|support|status|fs

Tapez help. llm=%s. phase3_complete=false. us031_complete=false.
""" % LLM_KIND

AI_HELP_TEXT = """=== Guide IA (mapping guest ai-help) ===
Guest Ring 3 : ai <texte> appelle SYS_GPT2_GENERATE ou GGUF.
Ici : llm=stub_echo. Pas un LLM de production. Pas OpenAI.

  ai <question>      - stub + scene HTML #ai-stage
  ai-mode [on|off]
  ai-runtime         - moteur=stub, live_guest=false
  ai-continue        - non attache (pas de session GGUF)

Exemples :
  ai comment lister initrd/bin/ ?
  ai dessine trois boites
  ai simule un acte VFS
aihelp ok
"""

VFS_TREE = {
    "": ["initrd/", "overlay/", "fat16/", "fat32/"],
    "initrd/": ["bin/", "hello.txt"],
    "initrd/bin/": ["shell", "vfsserver", "vfsvirtual", "idle", "fake_ai"],
    "overlay/": [],
    "fat16/": ["README.TXT"],
    "fat32/": ["README.TXT"],
}

VFS_FILES = {
    "initrd/hello.txt": "hello from bootstrap VFS (not guest initrd)\n",
    "fat16/README.TXT": "FAT16 bootstrap note. Guest mutate: vfsserver Ring 3.\n",
    "fat32/README.TXT": "FAT32 bootstrap note. Pas ext2.\n",
    "vfs-info": "vfs bootstrap osui ; guest mediateur=vfsserver\n",
    "vfs-mounts": "initrd/ ro\noverlay/ rw\nfat16/ rw\nfat32/ rw\n",
    "vfs-stats": "reads=0 writes=0 removes=0 renames=0\n",
}


def _join_cwd(cwd: str, path: str) -> str:
    raw = (path or "").strip()
    if not raw or raw == ".":
        return cwd
    if raw.startswith("/"):
        raw = raw.lstrip("/")
    if cwd and not raw.startswith(("initrd", "overlay", "fat16", "fat32", "vfs-")):
        if cwd.endswith("/"):
            raw = cwd + raw
        else:
            raw = cwd + "/" + raw
    if raw.endswith("/") and raw not in VFS_TREE and raw.rstrip("/") + "/" in VFS_TREE:
        raw = raw.rstrip("/") + "/"
    return raw


class MultibootShell:
    """Interpreteur bootstrap aligne sur le shell ELF Ring 3."""

    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        self.attachment = ATTACHMENT_BOOTSTRAP
        self.cwd = "initrd/"
        self.last_rc = 0
        self.ai_mode = True
        self.history: list[str] = []
        self.env = {
            "SHELL": SOURCE,
            "LLM": LLM_KIND,
            "ATTACHMENT": ATTACHMENT_BOOTSTRAP,
        }

    def public_meta(self) -> dict:
        return {
            "prompt": PROMPT,
            "attachment": self.attachment,
            "live_guest": self.attachment == ATTACHMENT_LIVE,
            "qemu_serial": False,
            "source": SOURCE,
            "commands": list(GUEST_COMMANDS),
            "linux_bash": False,
            "note": (
                "Meme vocabulaire que le shell Multiboot Ring 3. "
                "Live QEMU/serial non branche dans osui."
            ),
        }

    def execute(self, line: str) -> dict:
        raw = (line or "").rstrip("\n")
        trimmed = raw.strip()
        if trimmed:
            self.history.append(trimmed)
            if len(self.history) > 64:
                self.history = self.history[-64:]
        if not trimmed:
            self.last_rc = 0
            return self._result("", 0)

        if trimmed.startswith("/"):
            return self._slash_or_open(trimmed)

        parts = trimmed.split()
        cmd = parts[0].lower()
        args = parts[1:]

        if cmd in LINUX_TRAPS:
            self.last_rc = 1
            return self._result(
                "Pas un bash Linux. Shell Multiboot (%s). Tapez help.\n" % SOURCE,
                1,
            )

        handler = self._handlers().get(cmd)
        if handler is None:
            self.last_rc = 1
            return self._result(
                "commande inconnue. help pour la liste (Multiboot Ring 3, pas un bash).\n",
                1,
            )
        output, rc, extra = handler(args)
        self.last_rc = rc
        payload = self._result(output, rc)
        if extra:
            payload.update(extra)
        return payload

    def _result(self, output: str, rc: int) -> dict:
        return {
            "status": "ok" if rc == 0 else "error",
            "output": output,
            "rc": rc,
            "prompt": PROMPT,
            "attachment": self.attachment,
            "live_guest": False,
            "llm": LLM_KIND,
            "cwd": self.cwd,
        }

    def _handlers(self):
        return {
            "help": self._cmd_help,
            "?": self._cmd_help,
            "ls": self._cmd_ls,
            "dir": self._cmd_ls,
            "pwd": self._cmd_pwd,
            "cd": self._cmd_cd,
            "whoami": self._cmd_whoami,
            "sysinfo": self._cmd_sysinfo,
            "info": self._cmd_sysinfo,
            "ps": self._cmd_ps,
            "mem": self._cmd_mem,
            "memory": self._cmd_mem,
            "echo": self._cmd_echo,
            "clear": self._cmd_clear,
            "cls": self._cmd_clear,
            "which": self._cmd_which,
            "rc": self._cmd_rc,
            "date": self._cmd_date,
            "uptime": self._cmd_uptime,
            "getpid": self._cmd_getpid,
            "history": self._cmd_history,
            "env": self._cmd_env,
            "cat": self._cmd_cat,
            "stat": self._cmd_stat,
            "ai": self._cmd_ai,
            "ai-help": self._cmd_ai_help,
            "aihelp": self._cmd_ai_help,
            "ai-mode": self._cmd_ai_mode,
            "aimode": self._cmd_ai_mode,
            "ai-stats": self._cmd_ai_stats,
            "aistats": self._cmd_ai_stats,
            "ai-provider": self._cmd_ai_provider,
            "ai-runtime": self._cmd_ai_runtime,
            "ai-model": self._cmd_ai_model,
            "ai-continue": self._cmd_ai_continue,
            "ai-test": self._cmd_ai_test,
            "aitest": self._cmd_ai_test,
            "vfs-list": self._cmd_vfs_list,
            "vfs-read": self._cmd_vfs_read,
            "vfs-stat": self._cmd_vfs_stat,
            "vfs-stats": self._cmd_vfs_stats,
            "vfs-write": self._cmd_vfs_mutate_denied,
            "vfs-remove": self._cmd_vfs_mutate_denied,
            "vfs-rename": self._cmd_vfs_mutate_denied,
            "vfs-mkdir": self._cmd_vfs_mutate_denied,
            "vfs-rmdir": self._cmd_vfs_mutate_denied,
            "vfs-mount-add": self._cmd_vfs_mutate_denied,
            "vfs-mount-remove": self._cmd_vfs_mutate_denied,
            "fat16-list": self._cmd_fat16_list,
            "fat16-cat": self._cmd_fat16_cat,
            "net-status": self._cmd_net_status,
            "spawn": self._cmd_not_live,
            "yield": self._cmd_not_live,
            "ipc-send": self._cmd_not_live,
            "ipc-recv": self._cmd_not_live,
            "service-find": self._cmd_not_live,
            "service-status": self._cmd_not_live,
            "mkdir": self._cmd_vfs_mutate_denied,
            "rmdir": self._cmd_vfs_mutate_denied,
            "rm": self._cmd_vfs_mutate_denied,
            "touch": self._cmd_vfs_mutate_denied,
            "write": self._cmd_vfs_mutate_denied,
            "append": self._cmd_vfs_mutate_denied,
            "attach": self._cmd_attach,
            "detach": self._cmd_detach,
            "guest-status": self._cmd_guest_status,
            "open": self._cmd_open,
        }

    def _banner_line(self) -> str:
        return "attachment=%s live_guest=false qemu_serial=false\n" % self.attachment

    def _cmd_help(self, args: list[str]):
        return HELP_TEXT if HELP_TEXT.endswith("\n") else HELP_TEXT + "\n", 0, None

    def _cmd_ls(self, args: list[str]):
        path = _join_cwd(self.cwd, args[0] if args else "")
        if path and not path.endswith("/") and path + "/" in VFS_TREE:
            path = path + "/"
        names = VFS_TREE.get(path)
        if names is None:
            names = VFS_TREE.get(path + "/")
            path = path + "/" if names is not None else path
        if names is None:
            return "ls: introuvable (miroir bootstrap, pas l'initrd guest)\n", 1, None
        listing = " ".join(names) if names else "(vide)"
        return (
            "vfs bootstrap · mapping guest vfs-list / ls\n%s\n" % listing,
            0,
            None,
        )

    def _cmd_pwd(self, args: list[str]):
        return self.cwd + "\n", 0, None

    def _cmd_cd(self, args: list[str]):
        if not args:
            self.cwd = "initrd/"
            return self.cwd + "\n", 0, None
        path = _join_cwd(self.cwd, args[0])
        if path and not path.endswith("/"):
            path = path + "/"
        if path not in VFS_TREE:
            return "cd: pas un repertoire du miroir bootstrap\n", 1, None
        self.cwd = path
        return self.cwd + "\n", 0, None

    def _cmd_whoami(self, args: list[str]):
        return "shell Ring 3 Multiboot (bootstrap osui) user=operator\n", 0, None

    def _cmd_sysinfo(self, args: list[str]):
        text = (
            "sysinfo bootstrap osui\n"
            "product=Mohhdy OS (un SE Multiboot)\n"
            "shell=%s prompt=%s\n"
            "llm=%s phase3_complete=false us031_complete=false\n"
            "guest_live=false qemu_in_container=false\n"
            "%s"
        ) % (SOURCE, PROMPT, LLM_KIND, self._banner_line())
        return text, 0, None

    def _cmd_ps(self, args: list[str]):
        text = (
            "PID  NAME       STATE   note\n"
            "1    osui       RUN     chrome graphique bootstrap\n"
            "2    agent      RUN     backend temporaire\n"
            "(pas la table de taches guest ; spawn live non attache)\n"
        )
        return text, 0, None

    def _cmd_mem(self, args: list[str]):
        return "mem bootstrap : processus hote Python. Pas PMM/VMM guest.\n", 0, None

    def _cmd_echo(self, args: list[str]):
        return (" ".join(args) + "\n") if args else "\n", 0, None

    def _cmd_clear(self, args: list[str]):
        return "", 0, {"clear": True}

    def _cmd_which(self, args: list[str]):
        if not args:
            return "which: commande manquante\n", 1, None
        name = args[0]
        if name in GUEST_COMMANDS or name in LINUX_TRAPS:
            if name in LINUX_TRAPS:
                return "which: pas un builtin Multiboot\n", 1, None
            return "which ok builtin %s\n" % name, 0, None
        return "which ok bin/%s\n" % name, 0, None

    def _cmd_rc(self, args: list[str]):
        return "rc %d\n" % self.last_rc, 0, None

    def _cmd_date(self, args: list[str]):
        return "date bootstrap (horloge hote, pas RTC guest)\n", 0, None

    def _cmd_uptime(self, args: list[str]):
        return "uptime bootstrap osui\n", 0, None

    def _cmd_getpid(self, args: list[str]):
        return "getpid bootstrap=1 (pas SYS_GETPID guest)\n", 0, None

    def _cmd_history(self, args: list[str]):
        if not self.history:
            return "(vide)\n", 0, None
        lines = ["%d  %s" % (i + 1, row) for i, row in enumerate(self.history)]
        return "\n".join(lines) + "\n", 0, None

    def _cmd_env(self, args: list[str]):
        lines = ["%s=%s" % (k, v) for k, v in sorted(self.env.items())]
        return "\n".join(lines) + "\n", 0, None

    def _cmd_cat(self, args: list[str]):
        if not args:
            return "cat: fichier manquant\n", 1, None
        path = _join_cwd(self.cwd, args[0])
        data = VFS_FILES.get(path)
        if data is None:
            return "cat: fichier introuvable (miroir bootstrap)\n", 1, None
        return data if data.endswith("\n") else data + "\n", 0, None

    def _cmd_stat(self, args: list[str]):
        if not args:
            return "stat: chemin manquant\n", 1, None
        path = _join_cwd(self.cwd, args[0])
        if path in VFS_FILES:
            return "stat file size=%d %s\n" % (len(VFS_FILES[path]), path), 0, None
        key = path if path.endswith("/") else path + "/"
        if key in VFS_TREE:
            return "stat dir %s\n" % key, 0, None
        return "stat: introuvable\n", 1, None

    def _cmd_ai(self, args: list[str]):
        question = " ".join(args).strip()
        if not question:
            return "Usage: ai <question>\n", 1, None
        stage = reason_stage(question)
        text = (
            "ai stub llm=%s (guest: SYS_GPT2_GENERATE / GGUF)\n"
            "mode_scene=%s\n"
            "%s\n"
        ) % (LLM_KIND, stage["mode"], stage["note"])
        return text, 0, {"stage": stage}

    def _cmd_ai_help(self, args: list[str]):
        return AI_HELP_TEXT if AI_HELP_TEXT.endswith("\n") else AI_HELP_TEXT + "\n", 0, None

    def _cmd_ai_mode(self, args: list[str]):
        if args and args[0] in ("on", "off"):
            self.ai_mode = args[0] == "on"
        return "ai-mode %s (stub osui, pas le flag guest)\n" % (
            "on" if self.ai_mode else "off"
        ), 0, None

    def _cmd_ai_stats(self, args: list[str]):
        return "ai-stats stub llm=%s prompts=%d\n" % (
            LLM_KIND,
            sum(1 for row in self.history if row.startswith("ai ")),
        ), 0, None

    def _cmd_ai_provider(self, args: list[str]):
        if args and args[0].lower() == "openai":
            return (
                "ai-provider: openai refuse (meme honnetete que le guest). "
                "local stub seulement. Pas d'appel reseau.\n"
            ), 1, None
        return "ai-provider local stub_echo\n", 0, None

    def _cmd_ai_runtime(self, args: list[str]):
        text = (
            "ai-runtime\n"
            "  llm=%s\n"
            "  guest_mapping=SYS_GPT2_GENERATE / GGUF session ai / ai-continue\n"
            "  live_guest=false\n"
            "  qemu_serial=false\n"
            "  phase3_complete=false\n"
            "  us031_complete=false\n"
        ) % LLM_KIND
        return text, 0, None

    def _cmd_ai_model(self, args: list[str]):
        return (
            "ai-model stub. Guest catalogue: gpt2_124M.bin / gpt2.gguf. "
            "Non charge dans osui.\n"
        ), 0, None

    def _cmd_ai_continue(self, args: list[str]):
        return (
            "ai-continue: session GGUF non attachee (bootstrap). "
            "Live guest requis pour un vrai token.\n"
        ), 1, None

    def _cmd_ai_test(self, args: list[str]):
        return "aitest ok stub (pas exec bin/ai_assistant guest)\n", 0, None

    def _vfs_note(self) -> str:
        return "vfs bootstrap osui ; guest: %s -> vfsserver\n" % SOURCE

    def _cmd_vfs_list(self, args: list[str]):
        path = args[0] if args else self.cwd
        if not path.endswith("/"):
            return "vfs-list: le chemin doit finir par / (contrat guest)\n", 1, None
        names = VFS_TREE.get(path)
        if names is None:
            return self._vfs_note() + "vfs-list: hors montage du miroir\n", 1, None
        body = "\n".join(names) if names else "(vide)"
        return self._vfs_note() + body + "\n", 0, None

    def _cmd_vfs_read(self, args: list[str]):
        if not args:
            return "vfs-read: chemin manquant\n", 1, None
        path = args[0]
        data = VFS_FILES.get(path)
        if data is None:
            return self._vfs_note() + "vfs-read: introuvable\n", 1, None
        return self._vfs_note() + data, 0, None

    def _cmd_vfs_stat(self, args: list[str]):
        if not args:
            return "vfs-stat: chemin manquant\n", 1, None
        path = args[0]
        if path in VFS_FILES:
            return (
                self._vfs_note()
                + "status=ok type=file size=%d\n" % len(VFS_FILES[path]),
                0,
                None,
            )
        key = path if path.endswith("/") else path + "/"
        if key in VFS_TREE:
            return self._vfs_note() + "status=ok type=dir %s\n" % key, 0, None
        return self._vfs_note() + "status=not_found\n", 1, None

    def _cmd_vfs_stats(self, args: list[str]):
        return self._vfs_note() + VFS_FILES["vfs-stats"], 0, None

    def _cmd_vfs_mutate_denied(self, args: list[str]):
        return (
            self._vfs_note()
            + "mutation refusee sur le miroir bootstrap. "
            "Le guest execute ces commandes via overlay/FAT sous capacite.\n"
        ), 1, None

    def _cmd_fat16_list(self, args: list[str]):
        names = VFS_TREE.get("fat16/") or []
        return (
            "fat16-list bootstrap (guest: SYS FAT16 racine)\n"
            + " ".join(names)
            + "\n"
        ), 0, None

    def _cmd_fat16_cat(self, args: list[str]):
        name = args[0] if args else "README.TXT"
        path = name if name.startswith("fat16/") else "fat16/" + name
        data = VFS_FILES.get(path)
        if data is None:
            return "fat16-cat: introuvable\n", 1, None
        return data, 0, None

    def _cmd_net_status(self, args: list[str]):
        return (
            "net-status nic=absent (pas de NE2000 dans osui). "
            "Guest: SYS_NET_STATUS / qemu-ne2k-*.\n"
        ), 0, None

    def _cmd_not_live(self, args: list[str]):
        return (
            "commande guest connue, non executee : live QEMU/serial "
            "non branche (attachment=bootstrap).\n"
        ), 1, None

    def _cmd_attach(self, args: list[str]):
        return (
            "attach: live QEMU/serial n'est pas branche dans ce processus osui.\n"
            "Etat: bootstrap. Le guest Multiboot (%s) n'est pas attache.\n"
            "Futur: attache serie/HMP sans pretendre que c'est deja le cas.\n"
        ) % SOURCE, 1, None

    def _cmd_detach(self, args: list[str]):
        self.attachment = ATTACHMENT_BOOTSTRAP
        self.env["ATTACHMENT"] = ATTACHMENT_BOOTSTRAP
        return "detach: deja bootstrap. live_guest=false\n", 0, None

    def _cmd_guest_status(self, args: list[str]):
        return (
            "guest-status attachment=%s live_guest=false qemu_serial=false\n"
            "source=%s prompt=%s\n"
            "scene_html=#ai-stage (osui seulement, pas le guest VGA)\n"
        ) % (self.attachment, SOURCE, PROMPT), 0, None

    def _cmd_open(self, args: list[str]):
        target = (args[0] if args else "").lower()
        if target not in OSUI_PANES:
            return "usage: open browser|shell|admin|support|status|fs\n", 1, None
        return "ouvert: %s (fenetre osui, hors guest)\n" % target, 0, {
            "open_pane": target
        }

    def _slash_or_open(self, trimmed: str) -> dict:
        token = trimmed.split()[0][1:].lower()
        mapping = {
            "browser": "browser",
            "shell": "shell",
            "admin": "admin",
            "support": "support",
            "status": "status",
            "fs": "fs",
            "help": None,
        }
        if token == "help":
            output, rc, extra = self._cmd_help([])
            self.last_rc = rc
            payload = self._result(output, rc)
            if extra:
                payload.update(extra)
            return payload
        pane = mapping.get(token)
        if pane:
            self.last_rc = 0
            payload = self._result("ouvert: /%s (fenetre osui)\n" % token, 0)
            payload["open_pane"] = pane
            return payload
        self.last_rc = 1
        return self._result("Commande inconnue /%s. Tapez help.\n" % token, 1)
