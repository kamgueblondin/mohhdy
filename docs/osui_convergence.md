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
| `userspace/osui_runtime.c` | Runtime OS-UI (chat, scene VGA, sessions, MCP, FS) |
| `shared/multiboot_shell_commands.json` | Registre genere (184 noms) |
| `userspace/mohhdy_osui_bridge.h` | `GUEST_HTML_STAGE 0`, `STAGE_VGA 1`, `PYTHON_FACADE 0` |
| `scripts/extract_guest_commands.py` | Regenere JSON + header ; `--check` dans `make test-all` |

Ne pas editer JSON/header a la main.

```text
python3 scripts/extract_guest_commands.py
make osui-registry-check
```

## Ce qui est dans le guest

- Chat / prompt stub, slash, pieges Linux
- Scene VGA 8x48, modes reflecting / acting / presenting
- Sessions `s0001+`, grant/revoke, escalate/takeover
- Origine, gestes simulateur, MCP declare, FS lecture
- `live_guest=true` : on **est** le guest (plus d'attache hote Python)

## Ce qui reste hors C freestanding

- Framebuffer HTML / `#ai-stage` (ETAT_REEL : scene structuree seulement)
- Navigateur-OS Chromium (US-031, `us031_complete=false`)
- LLM de production (`llm=stub_echo` ; GPT-2 local = autre chemin `ai`)
- Widget embed HTTP pour un site tiers

Interdit : declarer que le VGA heberge `#ai-stage`. Interdit d'allonger
`make integration-qemu`. Interdit OpenAI en CI.

Guides : [osui_0_1_2.md](osui_0_1_2.md), [osui_shell_live.md](osui_shell_live.md),
[osui_ai_stage.md](osui_ai_stage.md), [PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md).
