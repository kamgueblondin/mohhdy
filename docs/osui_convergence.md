# Convergence vers le SE Multiboot unique

**Date :** 16 septembre 2026
**Statut :** pont de code (registre + header), pas le retrait de `agent/`
**Ponctuation :** ASCII usuel et accents francais uniquement

Mohhdy est **un** produit : le **Multiboot Mohhdy OS**. Chat dirige par
prompts, chat flottant, navigateur-OS, scene IA HTML et shell Ring 3
sont des **devoirs de cet OS**. `osui/` / Docker sont le **bootstrap
graphique** qui converge vers ce SE, pas un second produit.

Cette page dit ou les features doivent **atterrir** : dans le SE
Multiboot. Elle separe honnetement bootstrap et guest.

## Artefact de pont (vrai linkage)

| Fichier | Role |
|---|---|
| `userspace/shell.c` | Source de verite des builtins Ring 3 |
| `shared/multiboot_shell_commands.json` | Registre genere (148 noms) |
| `userspace/mohhdy_osui_bridge.h` | Header guest : `MOHHDY_OSUI_GUEST_HTML_STAGE 0`, liste C |
| `osui/command_registry.py` | Charge le JSON pour `/shell` et `GET /api/os/commands` |
| `osui/scripts/extract_guest_commands.py` | Regenere JSON + header ; `--check` dans `make osui-smoke` |

Ne pas editer JSON/header a la main. Si `shell.c` gagne une commande,
regenerer :

```text
python3 osui/scripts/extract_guest_commands.py
```

`make osui-smoke` echoue si le registre est stale.

## Ce qui reste bootstrap (`osui/`)

- Chrome HTML, chat central / flottant, panes Admin / Support / Browser-OS
- Scene IA `#ai-stage` (HTML/SVG stub, allowlist, mini-plans)
- Backend HTTP `agent/` (sessions, droits, gestes simulateur)
- Interpreteur `/shell` hors attache live
- Prompt OS (`POST /api/os/prompt`) : routes NL vers panes / scene / builtins surs

## Ce qui doit entrer dans le guest (pas livre ici)

- Framebuffer / scene HTML dans le VGA (`guest_html_stage=false`, ETAT_REEL)
- Chat UI natif Ring 3 (pas le widget Python)
- Navigateur-OS (US-031, `us031_complete=false`)
- TTY live par defaut dans Docker (l'attache est un hook, pas le boot conteneur)
- Retrait de `agent/` (OS-UI-3, parite d'abord)

## Features land in Multiboot SE

Ordre vise :

1. Vocabulaire unique (cette tranche : registre)
2. Hook live `/shell` -> QEMU serial/HMP (cette tranche : chemin, defaut bootstrap)
3. Scene IA plus riche cote bootstrap, puis portage framebuffer guest **mesure**
4. Chat / Browser-OS comme surfaces du SE, plus comme recit sidecar
5. OS-UI-3 : facade Python retiree apres checklist de parite

Interdit : declarer que le VGA guest heberge deja `#ai-stage`. Interdit
d'allonger `make integration-qemu`. Interdit OpenAI en CI.

Guides : [osui_0_1_2.md](osui_0_1_2.md), [osui_shell_live.md](osui_shell_live.md),
[osui_ai_stage.md](osui_ai_stage.md), [PLAN_SE_MOHHDY_COMPLET.md](PLAN_SE_MOHHDY_COMPLET.md).
