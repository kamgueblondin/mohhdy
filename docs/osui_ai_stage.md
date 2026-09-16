# OS-UI : scene IA

**Date :** 16 septembre 2026
**Statut :** etat scene dans le guest Ring 3 ; rendu VBE QEMU (kind/mode)
**Ponctuation :** ASCII usuel et accents francais uniquement

Le **produit final** est un seul SE : le **Multiboot Mohhdy OS**. La
scene IA est un **devoir de cet OS**. L'etat (`stage_mode`, `stage_kind`,
prompt) vit dans `osui_runtime.c`. Le rendu glassmorphic (panneau
Scene IA, reflexion / action / resultats, constructions pixel) est
`kernel/gfx_desktop.c` dans la fenetre QEMU.

[ETAT_REEL.md](ETAT_REEL.md) mesure le guest. Le guest **n'execute pas**
HTML : `MOHHDY_OSUI_GUEST_HTML_STAGE 0`, `MOHHDY_OSUI_DISPLAY_HOST 0`.

## Modele

Les prompts (`chat`, `prompt`, `stage-prompt`, `/plan`, `/draw`)
classifient `kind` (`circle`, `boxes`, `graph`, `tree`, `clock`, `sim`,
`plan`) et `mode` (`reflecting` / `acting` / `presenting`). L'instantane
`OSUI-SNAP` porte ces champs. Le JS hote les dessine. Ce n'est **pas**
un moteur Chromium ni un LLM de production.

| Mode | Role |
|---|---|
| `reflecting` | Le stub lit le prompt et expose un plan |
| `acting` | Simulation courte etiquetee stub |
| `presenting` | Resultat structure (boites A/B/C, cercle, graphe, etc.) |

Mini-plan : `/plan` enchaine acting. `llm=stub_echo`. Les balises
`<script>` sont strippees (`scripts_stripped=1`).

## Commandes

```text
stage
stage-prompt dessine trois boites
/plan
chat bonjour
os-status
```

Sortie : `mode=`, `kind=`, `guest_html_stage=false`, `llm=stub_echo`.
Instantane nographic : `gui-status`. Bureau : `make run-gui`.

Guides : [osui_0_1_2.md](osui_0_1_2.md), [osui_chat_desktop.md](osui_chat_desktop.md).
