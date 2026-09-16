# Convergence vers le SE Multiboot unique

**Date :** 16 septembre 2026
**Statut :** OS-UI-C + OS-UI-3 livres. Surface = guest C. Facade Python retiree
**Ponctuation :** ASCII usuel et accents francais uniquement

Mohhdy est **un** produit : le **Multiboot Mohhdy OS**. Chat dirige par
prompts, scene IA, sessions, droits, gestes et shell Ring 3 atterrissent
dans **ce** SE. Docker boot QEMU. Il n'y a plus de sidecar HTTP.

## Artefact de pont (vrai linkage)

| Fichier | Role |
|---|---|
| `userspace/shell.c` | Source de verite des builtins Ring 3 |
| `userspace/osui_runtime.c` | Runtime OS-UI (chat, scene VGA, sessions, MCP, FS, commande `gui`) |
| `userspace/osui_gui.c` | Boucle gui + instantanes `OSUI-SNAP` (meme etat que le runtime) |
| `osui/static` | Surface HTML/CSS/JS (chrome glassmorphic) |
| `osui/display_host.py` | Helper mince : static + proxy serie, pas de logique metier |
| `shared/multiboot_shell_commands.json` | Registre genere (191 noms) |
| `userspace/mohhdy_osui_bridge.h` | `GUEST_HTML_STAGE 0`, `DISPLAY_HOST 1`, `GUI_COMMAND "gui"`, `PYTHON_FACADE 0` |
| `scripts/extract_guest_commands.py` | Regenere JSON + header ; `--check` dans `make test-all` |

Ne pas editer JSON/header a la main.

```text
python3 scripts/extract_guest_commands.py
make osui-registry-check
```

## Ce qui est dans le guest

- Chat / prompt stub, slash, pieges Linux
- Scene kind/mode (reflecting / acting / presenting)
- Commande `gui` / `graphics` / `desktop` ; `console` pour quitter
- Surface HTML hote (display_host), cerveau C
- Sessions `s0001+`, grant/revoke, escalate/takeover
- Origine, gestes simulateur, MCP declare, FS lecture
- `live_guest=true` : on **est** le guest (plus d'attache hote Python)

## Ce qui reste hors C freestanding

- Framebuffer execute **dans** le guest i386 (`guest_html_stage=false`)
- Navigateur-OS Chromium (US-031, `us031_complete=false`)
- LLM de production (`llm=stub_echo` ; GPT-2 local = autre chemin `ai`)
- Widget embed HTTP pour un site tiers

Interdit : reintroduire `agent/` comme cerveau. Interdit d'allonger
`make integration-qemu`. Interdit OpenAI en CI. Interdit de vendre US-031.

Guides : [osui_0_1_2.md](osui_0_1_2.md), [osui_shell_live.md](osui_shell_live.md),
[osui_ai_stage.md](osui_ai_stage.md), [PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md).
