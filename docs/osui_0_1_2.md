# OS-UI-0 / OS-UI-1 / OS-UI-2 / OS-UI-3 - surface guest C

**Date :** 16 septembre 2026
**Statut :** parite portee dans le guest Ring 3. Facade Python retiree (OS-UI-3)
**Ponctuation :** ASCII usuel et accents francais uniquement

Mohhdy est **un seul SE Multiboot**. Chat, scene IA, sessions, droits,
gestes allowlistes, MCP et FS sandbox vivent dans `userspace/osui_runtime.c`,
derriere le vocabulaire `MOHHDY>` de `userspace/shell.c`. Le bureau VGA
est `userspace/osui_gui.c` (commande canonique `gui`). Docker boot
**cette** instance QEMU. Ce n'est **pas** US-031, **pas** un LLM de
production, **pas** un bash Linux, **pas** un chrome HTML `#ai-stage`.

Plan maitre : [PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md).
Epiques : [../US/mohhdy_os_ui_migration.md](../US/mohhdy_os_ui_migration.md).
Interaction : [osui_chat_desktop.md](osui_chat_desktop.md).
Scene IA : [osui_ai_stage.md](osui_ai_stage.md).
Attache : [osui_shell_live.md](osui_shell_live.md).
Convergence : [osui_convergence.md](osui_convergence.md).
Guest mesure : [ETAT_REEL.md](ETAT_REEL.md).

## Ce que `docker run` ouvre

Image produit : `mohhdy-os`. Entree par defaut : QEMU nographic, console
serie (CI). Bureau VGA : `make run-gui` sur l'hote, ou
`/os/qemu-gui.sh` si `DISPLAY` est fourni.

```text
make all
make qemu-osui-runtime
make qemu-osui-gui
make osui-smoke
docker build -t mohhdy-os .
docker run --rm -it mohhdy-os
docker compose run --rm mohhdy-os
make run-gui
```

Dans le guest, apres `MOHHDY>` : `gui` (aliases `graphics`, `desktop`).
Quitter : `console` / ESC. Nographic : `gui-status` dump le canvas.

Pas de port 8080. Pas de `GET /health`. Le prompt guest `MOHHDY>` est
l'instance. Sante honnete : `os-status` et `guest-status`
(`llm=stub_echo`, `python_facade=false`, `phase3_complete=false`,
`us031_complete=false`).

Aucun secret dans l'image. Utilisateur du conteneur = processus QEMU.
Les poids GPT-2 ne sont pas dans l'image par defaut (`MOHHDY_RAM=256M`).

## Architecture (un produit, un userspace C)

```text
docker run -it mohhdy-os
        |
        +-- qemu-system-i386 (Multiboot)
              kernel + initrd + overlay
              shell ELF Ring 3
              osui_runtime.c + osui_gui.c
                chat / prompt / slash
                commande gui (aliases graphics, desktop)
                scene VGA 8x48 + canvas desktop 22x78
                sessions s0001+ , grant/revoke, escalate/takeover
                origin-check, browser-* simulateur, mcp-invoice
                fs-list / fs-read (write et traversal refuses)
```

`agent/` et le serveur Python `osui/` n'existent plus. Python hote
reste pour les tests et `scripts/extract_guest_commands.py`.

## OS-UI-0 - chrome guest

- Prompt `MOHHDY>`, slash `/help` `/browser` `/shell` `/admin` `/support`
  `/status` `/fs` `/plan` `/center` `/close`
- Commande canonique `gui` (aliases `graphics`, `desktop`) ; `console` quitte
- Scene VGA structuree + constructions ASCII du bureau (pas HTML)
- Vocabulaire Multiboot conserve (`ls`, `ai`, `vfs-*`, pieges Linux)

## OS-UI-1 - sessions / droits

- `session-new` / `session-use` / `session-status` / `session-list`
- `chat` / `prompt` : stub echo, `request_id`, `llm=stub_echo`
- `grant` / `revoke` / `escalate` / `takeover` / `admin-status`
- `origin-check` : origine etrangere = `origin_denied` + `status=403`

## OS-UI-2 - navigateur-OS allowliste

- `browser-click` / `browser-type` / `browser-pointer` (simulateur DOM)
- `mcp-invoice` declare ; `mcp-invoke` outil non declare = `tool_undeclared`
- `fs-list` / `fs-read` sandbox ; `..` = `traversal_denied` ; write refuse
- `phase3_complete=false`, `us031_complete=false`

## OS-UI-3 - Python retire

- `agent/` supprime
- serveur HTML `osui/` supprime
- Docker = boot QEMU
- `MOHHDY_OSUI_PYTHON_FACADE 0`

## Verification

```text
make test-all
make osui-smoke
make qemu-osui-runtime
make qemu-osui-gui
make integration-qemu
```

`make integration-qemu` reste les sept contrats, sans OS-UI.
Le contrat OS-UI est `make qemu-osui-runtime` ; la fumee bureau est
`make qemu-osui-gui` (job CI separe `osui-guest`).

## Ce qui n'a pas pu atterrir en C freestanding

- Chromium / Playwright / US-031
- LLM de production (stub echo + GPT-2 local deja dans le guest)
- HTML `#ai-stage`, embed.js HTTP, UUID de session
- Install PC / cloud-init de l'ancien sidecar HTTP
